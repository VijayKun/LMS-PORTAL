/**
 * book_service.c - Book catalog business logic.
 */
#include "services/book_service.h"

#include <string.h>

#include "utils/json_utils.h"

/*
 * Only the columns the catalog needs are selected, and rows are ordered by
 * title. If your Supabase "public.books" table uses different column names,
 * this is the ONE place to adjust them.
 */
#define BOOKS_SELECT_SQL                                                       \
  "SELECT id, title, author, isbn, category, publisher, "                      \
  "total_copies, available_copies "                                            \
  "FROM public.books ORDER BY title"

BookListStatus book_service_list(Database *db, char *out, size_t out_size, char *errbuf,
                                 size_t errlen) {
  if (out == NULL || out_size == 0) {
    return BOOK_LIST_BUFFER_TOO_SMALL;
  }

  if (!db_is_configured(db)) {
    return BOOK_LIST_DB_UNAVAILABLE;
  }

  DbResult result;
  if (!db_query(db, BOOKS_SELECT_SQL, NULL, 0, &result, errbuf, errlen)) {
    return BOOK_LIST_QUERY_FAILED;
  }

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
  if (total + 2 > out_size) {
    return BOOK_LIST_BUFFER_TOO_SMALL;
  }

  out[total] = '}';
  out[total + 1] = '\0';
  return BOOK_LIST_OK;
}
