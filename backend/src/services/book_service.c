/**
 * book_service.c - Book catalog business logic.
 */
#include "services/book_service.h"

#include <stdio.h>
#include <string.h>

#include "utils/json_utils.h"

/*
 * Only the columns the catalog needs are selected, and rows are ordered by
 * title. If your Supabase "public.books" table uses different column names,
 * this is the ONE place to adjust them.
 */
#define BOOK_COLUMNS \
  "id, title, author, isbn, category, publisher, total_copies, available_copies"

/* Same columns as the catalog list, restricted to one row. */
#define BOOK_BY_ID_SQL \
  "SELECT " BOOK_COLUMNS " FROM public.books WHERE id = $1"

#define BOOK_BY_ISBN_SQL \
  "SELECT " BOOK_COLUMNS " FROM public.books WHERE isbn = $1"

/* Turn a DbResult row 0 into {"book":{...}} in out. */
static BookGetStatus format_single_book(const DbResult *result, char *out, size_t out_size) {
  const char *prefix = "{\"book\":";
  size_t prefix_len = strlen(prefix);
  if (prefix_len + 3 > out_size) {
    return BOOK_GET_BUFFER_TOO_SMALL;
  }
  memcpy(out, prefix, prefix_len);

  int written = json_object_of_row((const char *const *)result->columns, result->kinds,
                                   (const char *const *)result->values, result->ncols,
                                   out + prefix_len, out_size - prefix_len);
  if (written < 0) {
    return BOOK_GET_BUFFER_TOO_SMALL;
  }

  size_t total = prefix_len + (size_t)written;
  if (total + 2 > out_size) {
    return BOOK_GET_BUFFER_TOO_SMALL;
  }

  out[total] = '}';
  out[total + 1] = '\0';
  return BOOK_GET_OK;
}

/* Shared body for get-by-id and get-by-isbn. */
static BookGetStatus book_get_where(Database *db, const char *sql, const char *value,
                                    char *out, size_t out_size, char *errbuf,
                                    size_t errlen) {
  if (out == NULL || out_size == 0) {
    return BOOK_GET_BUFFER_TOO_SMALL;
  }
  if (value == NULL || value[0] == '\0') {
    return BOOK_GET_NOT_FOUND;
  }
  if (!db_is_configured(db)) {
    return BOOK_GET_DB_UNAVAILABLE;
  }

  const char *params[1] = {value};
  DbResult result;
  if (!db_query(db, sql, params, 1, &result, errbuf, errlen)) {
    return BOOK_GET_QUERY_FAILED;
  }

  if (result.nrows == 0) {
    db_result_free(&result);
    return BOOK_GET_NOT_FOUND;
  }

  BookGetStatus status = format_single_book(&result, out, out_size);
  db_result_free(&result);
  return status;
}

BookListStatus book_service_search(Database *db, const BookListQuery *query, char *out,
                                   size_t out_size, char *errbuf, size_t errlen) {
  if (out == NULL || out_size == 0) {
    return BOOK_LIST_BUFFER_TOO_SMALL;
  }
  if (!db_is_configured(db)) {
    return BOOK_LIST_DB_UNAVAILABLE;
  }

  const char *q = (query != NULL) ? query->q : NULL;
  const char *category = (query != NULL) ? query->category : NULL;
  if (q != NULL && q[0] == '\0') {
    q = NULL;
  }
  if (category != NULL && category[0] == '\0') {
    category = NULL;
  }

  int limit = (query != NULL && query->limit > 0) ? query->limit : BOOKS_DEFAULT_LIMIT;
  if (limit > BOOKS_MAX_LIMIT) {
    limit = BOOKS_MAX_LIMIT;
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
    snprintf(where, sizeof(where), " WHERE (title ILIKE $%d OR author ILIKE $%d)", nparams,
             nparams);
  }
  if (category != NULL) {
    nparams++;
    params[nparams - 1] = category;
    size_t wlen = strlen(where);
    snprintf(where + wlen, sizeof(where) - wlen, "%s category = $%d",
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
           "SELECT " BOOK_COLUMNS " FROM public.books%s ORDER BY title LIMIT $%d OFFSET $%d",
           where, limit_ph, offset_ph);

  DbResult result;
  if (!db_query(db, sql, params, nparams, &result, errbuf, errlen)) {
    return BOOK_LIST_QUERY_FAILED;
  }

  char suffix[64];
  int suffix_len = snprintf(suffix, sizeof(suffix), ",\"limit\":%d,\"offset\":%d}", limit,
                            offset);

  const char *prefix = "{\"books\":";
  size_t prefix_len = strlen(prefix);
  if (prefix_len + 3 > out_size) {
    db_result_free(&result);
    return BOOK_LIST_BUFFER_TOO_SMALL;
  }
  memcpy(out, prefix, prefix_len);

  int written = json_array_of_objects((const char *const *)result.columns, result.kinds,
                                      (const char *const *)result.values, result.nrows,
                                      result.ncols, out + prefix_len, out_size - prefix_len);
  db_result_free(&result);

  if (written < 0) {
    return BOOK_LIST_BUFFER_TOO_SMALL;
  }

  size_t total = prefix_len + (size_t)written;
  if (total + (size_t)suffix_len + 1 > out_size) {
    return BOOK_LIST_BUFFER_TOO_SMALL;
  }

  memcpy(out + total, suffix, (size_t)suffix_len);
  out[total + (size_t)suffix_len] = '\0';
  return BOOK_LIST_OK;
}

BookListStatus book_service_list(Database *db, char *out, size_t out_size, char *errbuf,
                                 size_t errlen) {
  return book_service_search(db, NULL, out, out_size, errbuf, errlen);
}

BookGetStatus book_service_get(Database *db, const char *id, char *out, size_t out_size,
                               char *errbuf, size_t errlen) {
  return book_get_where(db, BOOK_BY_ID_SQL, id, out, out_size, errbuf, errlen);
}

BookGetStatus book_service_get_by_isbn(Database *db, const char *isbn, char *out,
                                       size_t out_size, char *errbuf, size_t errlen) {
  return book_get_where(db, BOOK_BY_ISBN_SQL, isbn, out, out_size, errbuf, errlen);
}
