/**
 * auth_service.c - Login and current-admin lookup.
 *
 * The `password_hash` column of public.admins is filled in by Milestone 3 (this
 * milestone). An admin whose hash is still NULL can never log in.
 */
#include "services/auth_service.h"

#include <stdio.h>
#include <string.h>

#include "security/password.h"

/*
 * The columns the login/me flows need, in order, so indexes below line up.
 * The LAST result column (password_hash) is consumed here and never leaves the
 * service.
 */
#define ADMIN_SELECT_SQL                                                        \
  "SELECT id, name, email, role, title, password_hash FROM public.admins"

static void fill_admin(const char *const *values, AuthAdmin *out) {
  snprintf(out->id, sizeof(out->id), "%s", values[0] ? values[0] : "");
  snprintf(out->name, sizeof(out->name), "%s", values[1] ? values[1] : "");
  snprintf(out->email, sizeof(out->email), "%s", values[2] ? values[2] : "");
  snprintf(out->role, sizeof(out->role), "%s", values[3] ? values[3] : "");
  snprintf(out->title, sizeof(out->title), "%s", values[4] ? values[4] : "");
}

AuthLoginStatus auth_service_login(Database *db, const char *email, const char *password,
                                   AuthAdmin *out_admin, char *errbuf, size_t errlen) {
  if (out_admin == NULL) {
    return AUTH_LOGIN_QUERY_FAILED;
  }
  memset(out_admin, 0, sizeof(*out_admin));

  if (email == NULL || email[0] == '\0' || password == NULL) {
    return AUTH_LOGIN_BAD_CREDENTIALS;
  }
  if (!db_is_configured(db)) {
    return AUTH_LOGIN_DB_UNAVAILABLE;
  }

  char sql[256];
  snprintf(sql, sizeof(sql), "%s WHERE email = $1 LIMIT 1", ADMIN_SELECT_SQL);

  const char *params[1] = {email};
  DbResult result;
  if (!db_query(db, sql, params, 1, &result, errbuf, errlen)) {
    return AUTH_LOGIN_QUERY_FAILED;
  }

  if (result.nrows == 0) {
    db_result_free(&result);
    return AUTH_LOGIN_BAD_CREDENTIALS; /* unknown email, same answer as bad pwd */
  }

  /* Columns: id, name, email, role, title, password_hash. */
  const char *hash = result.values[5];
  bool ok = (hash != NULL && hash[0] != '\0') &&
            password_verify(password, hash);
  if (!ok) {
    db_result_free(&result);
    return AUTH_LOGIN_BAD_CREDENTIALS;
  }

  fill_admin((const char *const *)result.values, out_admin);
  db_result_free(&result);
  return AUTH_LOGIN_OK;
}

AuthMeStatus auth_service_get_admin(Database *db, const char *id, AuthAdmin *out_admin,
                                    char *errbuf, size_t errlen) {
  if (out_admin == NULL) {
    return AUTH_ME_QUERY_FAILED;
  }
  memset(out_admin, 0, sizeof(*out_admin));

  if (id == NULL || id[0] == '\0') {
    return AUTH_ME_NOT_FOUND;
  }
  if (!db_is_configured(db)) {
    return AUTH_ME_DB_UNAVAILABLE;
  }

  char sql[256];
  snprintf(sql, sizeof(sql), "%s WHERE id = $1 LIMIT 1", ADMIN_SELECT_SQL);

  const char *params[1] = {id};
  DbResult result;
  if (!db_query(db, sql, params, 1, &result, errbuf, errlen)) {
    return AUTH_ME_QUERY_FAILED;
  }

  if (result.nrows == 0) {
    db_result_free(&result);
    return AUTH_ME_NOT_FOUND;
  }

  fill_admin((const char *const *)result.values, out_admin);
  db_result_free(&result);
  return AUTH_ME_OK;
}
