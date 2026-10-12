/**
 * student_service.c - Student directory business logic.
 */
#include "services/student_service.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/json_utils.h"

/*
 * Only the columns the directory needs are selected, and rows are ordered by
 * name. If your Supabase "public.students" table uses different column names,
 * this is the ONE place to adjust them.
 */
#define STUDENT_COLUMNS                                                        \
  "id, name, email, student_id, branch, program, semester, contact, "          \
  "joined_date, status"

/* Same columns as the directory list, restricted to one row by primary key. */
#define STUDENT_BY_ID_SQL                                                      \
  "SELECT " STUDENT_COLUMNS " FROM public.students WHERE id = $1"

StudentListStatus student_service_search(Database *db, const StudentListQuery *query,
                                         char *out, size_t out_size, char *errbuf,
                                         size_t errlen) {
  if (out == NULL || out_size == 0) {
    return STUDENT_LIST_BUFFER_TOO_SMALL;
  }
  if (!db_is_configured(db)) {
    return STUDENT_LIST_DB_UNAVAILABLE;
  }

  const char *q = (query != NULL) ? query->q : NULL;
  const char *branch = (query != NULL) ? query->branch : NULL;
  if (q != NULL && q[0] == '\0') {
    q = NULL;
  }
  if (branch != NULL && branch[0] == '\0') {
    branch = NULL;
  }

  int limit = (query != NULL && query->limit > 0) ? query->limit : STUDENTS_DEFAULT_LIMIT;
  if (limit > STUDENTS_MAX_LIMIT) {
    limit = STUDENTS_MAX_LIMIT;
  }
  int offset = (query != NULL && query->offset > 0) ? query->offset : 0;

  const char *params[4];
  char limit_buf[16];
  char offset_buf[16];
  char q_pattern[300];
  char where[192] = "";
  int nparams = 0;

  if (q != NULL) {
    snprintf(q_pattern, sizeof(q_pattern), "%%%s%%", q);
    nparams++;
    params[nparams - 1] = q_pattern;
    snprintf(where, sizeof(where),
             " WHERE (name ILIKE $%d OR email ILIKE $%d OR student_id ILIKE $%d)", nparams,
             nparams, nparams);
  }
  if (branch != NULL) {
    nparams++;
    params[nparams - 1] = branch;
    size_t wlen = strlen(where);
    snprintf(where + wlen, sizeof(where) - wlen, "%s branch = $%d",
             (wlen == 0) ? " WHERE" : " AND", nparams);
  }

  snprintf(limit_buf, sizeof(limit_buf), "%d", limit);
  nparams++;
  params[nparams - 1] = limit_buf;
  int limit_ph = nparams;

  snprintf(offset_buf, sizeof(offset_buf), "%d", offset);
  nparams++;
  params[nparams - 1] = offset_buf;
  int offset_ph = nparams;

  char sql[384];
  snprintf(sql, sizeof(sql),
           "SELECT " STUDENT_COLUMNS " FROM public.students%s ORDER BY name LIMIT $%d OFFSET $%d",
           where, limit_ph, offset_ph);

  DbResult result;
  if (!db_query(db, sql, params, nparams, &result, errbuf, errlen)) {
    return STUDENT_LIST_QUERY_FAILED;
  }

  char suffix[64];
  int suffix_len = snprintf(suffix, sizeof(suffix), ",\"limit\":%d,\"offset\":%d}", limit,
                            offset);

  const char *prefix = "{\"students\":";
  size_t prefix_len = strlen(prefix);
  if (prefix_len + 3 > out_size) {
    db_result_free(&result);
    return STUDENT_LIST_BUFFER_TOO_SMALL;
  }
  memcpy(out, prefix, prefix_len);

  int written = json_array_of_objects((const char *const *)result.columns, result.kinds,
                                      (const char *const *)result.values, result.nrows,
                                      result.ncols, out + prefix_len, out_size - prefix_len);
  db_result_free(&result);

  if (written < 0) {
    return STUDENT_LIST_BUFFER_TOO_SMALL;
  }

  size_t total = prefix_len + (size_t)written;
  if (total + (size_t)suffix_len + 1 > out_size) {
    return STUDENT_LIST_BUFFER_TOO_SMALL;
  }

  memcpy(out + total, suffix, (size_t)suffix_len);
  out[total + (size_t)suffix_len] = '\0';
  return STUDENT_LIST_OK;
}

StudentListStatus student_service_list(Database *db, char *out, size_t out_size,
                                       char *errbuf, size_t errlen) {
  return student_service_search(db, NULL, out, out_size, errbuf, errlen);
}

StudentGetStatus student_service_get(Database *db, const char *id, char *out,
                                     size_t out_size, char *errbuf, size_t errlen) {
  if (out == NULL || out_size == 0) {
    return STUDENT_GET_BUFFER_TOO_SMALL;
  }
  if (id == NULL || id[0] == '\0') {
    return STUDENT_GET_NOT_FOUND;
  }
  if (!db_is_configured(db)) {
    return STUDENT_GET_DB_UNAVAILABLE;
  }

  const char *params[1] = {id};
  DbResult result;
  if (!db_query(db, STUDENT_BY_ID_SQL, params, 1, &result, errbuf, errlen)) {
    return STUDENT_GET_QUERY_FAILED;
  }

  if (result.nrows == 0) {
    db_result_free(&result);
    return STUDENT_GET_NOT_FOUND;
  }

  const char *prefix = "{\"student\":";
  size_t prefix_len = strlen(prefix);
  if (prefix_len + 3 > out_size) {
    db_result_free(&result);
    return STUDENT_GET_BUFFER_TOO_SMALL;
  }
  memcpy(out, prefix, prefix_len);

  int written = json_object_of_row((const char *const *)result.columns, result.kinds,
                                   (const char *const *)result.values, result.ncols,
                                   out + prefix_len, out_size - prefix_len);
  db_result_free(&result);

  if (written < 0) {
    return STUDENT_GET_BUFFER_TOO_SMALL;
  }

  size_t total = prefix_len + (size_t)written;
  if (total + 2 > out_size) {
    return STUDENT_GET_BUFFER_TOO_SMALL;
  }

  out[total] = '}';
  out[total + 1] = '\0';
  return STUDENT_GET_OK;
}

/* ---- Create / update / delete ------------------------------------------ */

/* "" passes; otherwise it must be a real calendar date (YYYY-MM-DD). */
static bool valid_date(const char *s) {
  if (s == NULL || s[0] == '\0') {
    return true;
  }
  int year = 0, month = 0, day = 0;
  char extra = '\0';
  if (sscanf(s, "%4d-%2d-%2d%c", &year, &month, &day, &extra) != 3) {
    return false;
  }
  if (year < 1000 || year > 9999 || month < 1 || month > 12 || day < 1 || day > 31) {
    return false;
  }
  static const int days_in_month[12] = {31, 28, 31, 30, 31, 30,
                                        31, 31, 30, 31, 30, 31};
  int max_day = days_in_month[month - 1];
  if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) {
    max_day = 29;
  }
  return day <= max_day;
}

/* Optional positive integer field. "" is allowed; alike values are rejected. */
static bool parse_optional_int(const char *s, long min, long max, long *out) {
  if (s == NULL || s[0] == '\0') {
    return true;
  }
  char *end = NULL;
  long v = strtol(s, &end, 10);
  if (end == s || *end != '\0' || v < min || v > max) {
    return false;
  }
  *out = v;
  return true;
}

static bool is_valid_status(const char *s) {
  return s[0] == '\0' || strcmp(s, "active") == 0 || strcmp(s, "suspended") == 0;
}

/*
 * Business rules shared by create and update. `require_basic` forces the
 * name/email/student_id minimum (creation); updates only need one field.
 */
static bool validate_student(const StudentMutate *b, bool require_basic, char *errbuf,
                             size_t errlen) {
  if (b == NULL) {
    snprintf(errbuf, errlen, "student input is NULL");
    return false;
  }

  if (require_basic) {
    if (b->name[0] == '\0' || b->email[0] == '\0' || b->student_id[0] == '\0') {
      snprintf(errbuf, errlen, "name, email and student_id are required");
      return false;
    }
  } else {
    const char *const fields[] = {
        b->name, b->email, b->student_id,    b->branch,     b->program,
        b->semester, b->contact, b->joined_date, b->status,
    };
    bool any = false;
    for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); i++) {
      if (fields[i][0] != '\0') {
        any = true;
        break;
      }
    }
    if (!any) {
      snprintf(errbuf, errlen, "no editable fields provided");
      return false;
    }
  }

  long semester = 0;
  if (!parse_optional_int(b->semester, 1, 20, &semester)) {
    snprintf(errbuf, errlen, "invalid semester");
    return false;
  }
  if (!is_valid_status(b->status)) {
    snprintf(errbuf, errlen, "status must be 'active' or 'suspended'");
    return false;
  }
  if (!valid_date(b->joined_date)) {
    snprintf(errbuf, errlen, "invalid joined_date (expected YYYY-MM-DD)");
    return false;
  }
  return true;
}

/* The database generates PK ids; this is the only place that's assumed. */
#define STUDENT_INSERT_SQL                                                   \
  "INSERT INTO public.students"                                              \
  " (id, name, email, student_id, branch, program, semester, contact,"       \
  "  joined_date, status)"                                                   \
  " VALUES (gen_random_uuid()::text, $1, $2, $3, $4, $5, $6, $7, $8, $9)"    \
  " RETURNING id"

StudentCreateStatus student_service_create(Database *db, const StudentMutate *student,
                                           char *id_out, size_t id_size, char *errbuf,
                                           size_t errlen) {
  if (id_out == NULL || id_size == 0) {
    return STUDENT_CREATE_BAD_INPUT;
  }
  if (!validate_student(student, true, errbuf, errlen)) {
    return STUDENT_CREATE_BAD_INPUT;
  }
  if (!db_is_configured(db)) {
    return STUDENT_CREATE_DB_UNAVAILABLE;
  }

  /* Empty optional strings fall back to schema defaults; an empty
   * joined_date is passed as SQL NULL. */
  const char *params[9];
  params[0] = student->name;
  params[1] = student->email;
  params[2] = student->student_id;
  params[3] = student->branch[0] ? student->branch : "";
  params[4] = student->program[0] ? student->program : "";
  params[5] = student->semester[0] ? student->semester : "1";
  params[6] = student->contact[0] ? student->contact : "";
  params[7] = student->joined_date[0] ? student->joined_date : NULL;
  params[8] = student->status[0] ? student->status : "active";

  DbResult result;
  if (!db_query(db, STUDENT_INSERT_SQL, params, 9, &result, errbuf, errlen)) {
    if (db_has_sqlstate(errbuf, DB_SQLSTATE_UNIQUE_VIOLATION)) {
      return STUDENT_CREATE_DUPLICATE;
    }
    return STUDENT_CREATE_QUERY_FAILED;
  }

  StudentCreateStatus status = STUDENT_CREATE_QUERY_FAILED;
  if (result.nrows == 1 && result.values != NULL && result.values[0] != NULL) {
    snprintf(id_out, id_size, "%s", result.values[0]);
    status = STUDENT_CREATE_OK;
  }
  db_result_free(&result);
  return status;
}

/* Update only the columns whose StudentMutate field is non-empty. */
StudentUpdateStatus student_service_update(Database *db, const char *id,
                                           const StudentMutate *student, char *errbuf,
                                           size_t errlen) {
  if (id == NULL || id[0] == '\0') {
    return STUDENT_UPDATE_NOT_FOUND;
  }
  if (!validate_student(student, false, errbuf, errlen)) {
    return STUDENT_UPDATE_BAD_INPUT;
  }
  if (!db_is_configured(db)) {
    return STUDENT_UPDATE_DB_UNAVAILABLE;
  }

  char sql[1024];
  snprintf(sql, sizeof(sql), "UPDATE public.students SET");

  const char *params[10];
  int nparams = 0;

#define ADD_SET(col, field)                                                   \
  if ((field)[0] != '\0') {                                                   \
    strncat(sql, (nparams == 0) ? "" : ", ", sizeof(sql) - strlen(sql) - 1);   \
    char seg[64];                                                            \
    snprintf(seg, sizeof(seg), " %s = $%d", col, nparams + 1);               \
    strncat(sql, seg, sizeof(sql) - strlen(sql) - 1);                        \
    params[nparams++] = (field);                                             \
  }

  ADD_SET("name", student->name);
  ADD_SET("email", student->email);
  ADD_SET("student_id", student->student_id);
  ADD_SET("branch", student->branch);
  ADD_SET("program", student->program);
  ADD_SET("semester", student->semester);
  ADD_SET("contact", student->contact);
  ADD_SET("joined_date", student->joined_date);
  ADD_SET("status", student->status);

#undef ADD_SET

  char idph[16];
  snprintf(idph, sizeof(idph), "$%d", nparams + 1);
  char tail[64];
  snprintf(tail, sizeof(tail), " WHERE id = %s RETURNING id", idph);
  strncat(sql, tail, sizeof(sql) - strlen(sql) - 1);
  params[nparams++] = id;

  DbResult result;
  if (!db_query(db, sql, params, nparams, &result, errbuf, errlen)) {
    if (db_has_sqlstate(errbuf, DB_SQLSTATE_UNIQUE_VIOLATION)) {
      return STUDENT_UPDATE_DUPLICATE;
    }
    return STUDENT_UPDATE_QUERY_FAILED;
  }

  StudentUpdateStatus status = STUDENT_UPDATE_NOT_FOUND;
  if (result.nrows == 1) {
    status = STUDENT_UPDATE_OK;
  }
  db_result_free(&result);
  return status;
}

StudentDeleteStatus student_service_delete(Database *db, const char *id, char *errbuf,
                                           size_t errlen) {
  if (id == NULL || id[0] == '\0') {
    return STUDENT_DELETE_NOT_FOUND;
  }
  if (!db_is_configured(db)) {
    return STUDENT_DELETE_DB_UNAVAILABLE;
  }

  const char *params[1] = {id};
  DbResult result;
  if (!db_query(db, "DELETE FROM public.students WHERE id = $1 RETURNING id", params, 1,
                &result, errbuf, errlen)) {
    return STUDENT_DELETE_QUERY_FAILED;
  }

  StudentDeleteStatus status = STUDENT_DELETE_NOT_FOUND;
  if (result.nrows == 1) {
    status = STUDENT_DELETE_OK;
  }
  db_result_free(&result);
  return status;
}
