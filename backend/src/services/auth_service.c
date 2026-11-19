/**
 * auth_service.c - Admin and student authentication.
 *
 * Identifiers and emails are passed as bound parameters. Password verification
 * uses the shared scrypt helper.
 */
#include "services/auth_service.h"

#include <stdio.h>
#include <string.h>

#include "security/password.h"

static const char *cell(const DbResult *result, int row, int col) {
  if (result == NULL || row < 0 || col < 0 || row >= result->nrows || col >= result->ncols) {
    return NULL;
  }
  return result->values[row * result->ncols + col];
}

static void copy_field(char *dst, size_t size, const char *src) {
  if (dst == NULL || size == 0) {
    return;
  }
  snprintf(dst, size, "%s", src == NULL ? "" : src);
}

static void fill_admin(AuthUser *out, const DbResult *result, int row) {
  copy_field(out->id, sizeof(out->id), cell(result, row, 0));
  copy_field(out->name, sizeof(out->name), cell(result, row, 1));
  copy_field(out->email, sizeof(out->email), cell(result, row, 2));
  copy_field(out->role, sizeof(out->role), cell(result, row, 3));
  copy_field(out->title, sizeof(out->title), cell(result, row, 4));
  if (out->role[0] == '\0') {
    copy_field(out->role, sizeof(out->role), "admin");
  }
}

static void fill_student(AuthUser *out, const DbResult *result, int row) {
  const char *program = cell(result, row, 5);
  const char *branch = cell(result, row, 4);
  copy_field(out->id, sizeof(out->id), cell(result, row, 0));
  copy_field(out->name, sizeof(out->name), cell(result, row, 1));
  copy_field(out->email, sizeof(out->email), cell(result, row, 2));
  copy_field(out->role, sizeof(out->role), "student");
  if (program != NULL && program[0] != '\0') {
    copy_field(out->title, sizeof(out->title), program);
  } else if (branch != NULL && branch[0] != '\0') {
    copy_field(out->title, sizeof(out->title), branch);
  } else {
    copy_field(out->title, sizeof(out->title), "Student");
  }
}

static int password_matches(const DbResult *result, int row, int col, const char *password) {
  const char *hash = cell(result, row, col);
  return hash != NULL && hash[0] != '\0' && password != NULL &&
         password_verify(password, hash);
}

static AuthLoginStatus run_login(Database *db, const char *sql, const char *identifier,
                                 const char *password, AuthUser *out, int hash_col,
                                 int is_student, char *errbuf, size_t errlen) {
  const char *params[1] = {identifier};
  DbResult result;
  memset(&result, 0, sizeof(result));
  char db_err[256] = {0};

  if (!db_query(db, sql, params, 1, &result, db_err, sizeof(db_err))) {
    db_result_free(&result);
    if (db == NULL || !db->connected) {
      snprintf(errbuf, errlen, "Database is unavailable.");
      return AUTH_LOGIN_DB_UNAVAILABLE;
    }
    snprintf(errbuf, errlen, "Account lookup failed.");
    return AUTH_LOGIN_QUERY_FAILED;
  }

  AuthLoginStatus status = AUTH_LOGIN_BAD_CREDENTIALS;
  if (result.nrows > 0 && password_matches(&result, 0, hash_col, password)) {
    if (is_student) {
      fill_student(out, &result, 0);
    } else {
      fill_admin(out, &result, 0);
    }
    status = AUTH_LOGIN_OK;
  }
  db_result_free(&result);
  return status;
}

AuthLoginStatus auth_service_login(Database *db, const char *email, const char *password,
                                   AuthUser *out, char *errbuf, size_t errlen) {
  if (out == NULL) {
    snprintf(errbuf, errlen, "Authentication is unavailable.");
    return AUTH_LOGIN_QUERY_FAILED;
  }
  if (email == NULL || password == NULL || email[0] == '\0' || password[0] == '\0') {
    snprintf(errbuf, errlen, "Email and password are required.");
    return AUTH_LOGIN_BAD_CREDENTIALS;
  }
  if (db == NULL || !db->connected) {
    snprintf(errbuf, errlen, "Database is unavailable.");
    return AUTH_LOGIN_DB_UNAVAILABLE;
  }

  AuthLoginStatus status = run_login(
      db,
      "SELECT id, name, COALESCE(email, ''), COALESCE(role, 'admin'), "
      "COALESCE(title, '') AS title, password_hash "
      "FROM public.admins WHERE lower(email) = lower($1) LIMIT 1",
      email, password, out, 5, 0, errbuf, errlen);
  if (status == AUTH_LOGIN_OK || status == AUTH_LOGIN_DB_UNAVAILABLE) {
    return status;
  }

  return run_login(
      db,
      "SELECT id, name, COALESCE(email, ''), COALESCE(student_id, ''), "
      "COALESCE(branch, ''), COALESCE(program, ''), password_hash "
      "FROM public.students "
      "WHERE lower(email) = lower($1) OR lower(student_id) = lower($1) LIMIT 1",
      email, password, out, 6, 1, errbuf, errlen);
}

static AuthMeStatus load_user(Database *db, const char *id, int is_student, AuthUser *out,
                              char *errbuf, size_t errlen) {
  const char *sql =
      is_student
          ? "SELECT id, name, COALESCE(email, ''), COALESCE(student_id, ''), "
            "COALESCE(branch, ''), COALESCE(program, ''), password_hash "
            "FROM public.students WHERE id = $1 LIMIT 1"
          : "SELECT id, name, COALESCE(email, ''), COALESCE(role, 'admin'), "
            "COALESCE(title, '') AS title, password_hash "
            "FROM public.admins WHERE id = $1 LIMIT 1";
  const char *params[1] = {id};
  DbResult result;
  memset(&result, 0, sizeof(result));
  char db_err[256] = {0};

  if (!db_query(db, sql, params, 1, &result, db_err, sizeof(db_err))) {
    db_result_free(&result);
    if (db == NULL || !db->connected) {
      snprintf(errbuf, errlen, "Database is unavailable.");
      return AUTH_ME_DB_UNAVAILABLE;
    }
    snprintf(errbuf, errlen, "User lookup failed.");
    return AUTH_ME_QUERY_FAILED;
  }
  if (result.nrows == 0) {
    db_result_free(&result);
    return AUTH_ME_NOT_FOUND;
  }
  if (is_student) {
    fill_student(out, &result, 0);
  } else {
    fill_admin(out, &result, 0);
  }
  db_result_free(&result);
  return AUTH_ME_OK;
}

AuthMeStatus auth_service_get_user(Database *db, const char *id, const char *role, AuthUser *out,
                                   char *errbuf, size_t errlen) {
  if (out == NULL || id == NULL || id[0] == '\0') {
    snprintf(errbuf, errlen, "A user id is required.");
    return AUTH_ME_NOT_FOUND;
  }
  if (db == NULL || !db->connected) {
    snprintf(errbuf, errlen, "Database is unavailable.");
    return AUTH_ME_DB_UNAVAILABLE;
  }
  return load_user(db, id, role != NULL && strcmp(role, "student") == 0, out, errbuf, errlen);
}

AuthMeStatus auth_service_get_admin(Database *db, const char *id, AuthAdmin *out, char *errbuf,
                                    size_t errlen) {
  return auth_service_get_user(db, id, "admin", out, errbuf, errlen);
}
