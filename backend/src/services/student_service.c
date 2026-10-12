/**
 * student_service.c - Student directory business logic.
 */
#include "services/student_service.h"

#include <string.h>

#include "utils/json_utils.h"

/*
 * Only the columns the directory needs are selected, and rows are ordered by
 * name. If your Supabase "public.students" table uses different column names,
 * this is the ONE place to adjust them.
 */
#define STUDENTS_SELECT_SQL                                                    \
  "SELECT id, name, email, student_id, branch, program, semester, contact, "   \
  "joined_date, status "                                                       \
  "FROM public.students ORDER BY name"

/* Same columns as the directory list, restricted to one row by primary key. */
#define STUDENT_BY_ID_SQL                                                      \
  "SELECT id, name, email, student_id, branch, program, semester, contact, "   \
  "joined_date, status "                                                       \
  "FROM public.students WHERE id = $1"

StudentListStatus student_service_list(Database *db, char *out, size_t out_size,
                                       char *errbuf, size_t errlen) {
  if (out == NULL || out_size == 0) {
    return STUDENT_LIST_BUFFER_TOO_SMALL;
  }

  if (!db_is_configured(db)) {
    return STUDENT_LIST_DB_UNAVAILABLE;
  }

  DbResult result;
  if (!db_query(db, STUDENTS_SELECT_SQL, NULL, 0, &result, errbuf, errlen)) {
    return STUDENT_LIST_QUERY_FAILED;
  }

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
  if (total + 2 > out_size) {
    return STUDENT_LIST_BUFFER_TOO_SMALL;
  }

  out[total] = '}';
  out[total + 1] = '\0';
  return STUDENT_LIST_OK;
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
