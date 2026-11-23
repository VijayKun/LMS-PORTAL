/**
 * book_service.c - Book catalog business logic.
 */
#include "services/book_service.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/json_utils.h"

/*
 * Only the columns the catalog needs are selected, and rows are ordered by
 * title. If your Supabase "public.books" table uses different column names,
 * this is the ONE place to adjust them.
 */
#define BOOK_COLUMNS                                                                    \
  "id, title, author, isbn, category, publisher, publication_date, description, cover, " \
  "year, branch_id, total_copies, available_copies, google_metadata"

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

/* ---- Create / update / delete ------------------------------------------ */

/*
 * Parse an optional non-negative integer. An empty string means "not
 * provided" and is allowed. Returns false on anything that is not an integer
 * or is outside [min, max].
 */
static bool parse_optional_int(const char *s, long min, long max, long *out) {
  if (s == NULL || s[0] == '\0') {
    return true; /* not provided */
  }
  char *end = NULL;
  long v = strtol(s, &end, 10);
  if (end == s || *end != '\0' || v < min || v > max) {
    return false;
  }
  *out = v;
  return true;
}

/* Google Books often returns just a year or year-month; pad to a real date. */
static void normalize_partial_date(char *value, size_t cap) {
  if (value == NULL || cap == 0) return;
  size_t len = strlen(value);
  if (len == 4) {
    for (size_t i = 0; i < len; i++) {
      if (!isdigit((unsigned char)value[i])) return;
    }
    snprintf(value, cap, "%s-01-01", value);
  } else if (len == 7 && value[4] == '-') {
    for (size_t i = 0; i < len; i++) {
      if (i != 4 && !isdigit((unsigned char)value[i])) return;
    }
    snprintf(value, cap, "%s-01", value);
  }
}

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
    max_day = 29; /* leap year */
  }
  return day <= max_day;
}

/*
 * Business rules shared by create and update. `require_basic` forces the
 * title/isbn minimum (creation); updates only need at least one field.
 * Returns false and fills errbuf (server log only) on the first problem.
 */
static bool validate_book(const BookMutate *b, bool require_basic, char *errbuf,
                          size_t errlen) {
  if (b == NULL) {
    snprintf(errbuf, errlen, "book input is NULL");
    return false;
  }

  if (require_basic) {
    if (b->title[0] == '\0' || b->isbn[0] == '\0') {
      snprintf(errbuf, errlen, "title and isbn are required");
      return false;
    }
  } else {
    const char *const fields[] = {
        b->title,         b->author,       b->isbn,
        b->category,      b->publisher,    b->publication_date,
        b->description,   b->cover,        b->branch_id,
        b->total_copies,  b->available_copies, b->year,
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

  long total = -1, available = -1;
  if (!parse_optional_int(b->total_copies, 0, 1000000L, &total)) {
    snprintf(errbuf, errlen, "invalid total_copies");
    return false;
  }
  if (!parse_optional_int(b->available_copies, 0, 1000000L, &available)) {
    snprintf(errbuf, errlen, "invalid available_copies");
    return false;
  }
  if (total >= 0 && available >= 0 && available > total) {
    snprintf(errbuf, errlen, "available_copies exceeds total_copies");
    return false;
  }
  long year = 0;
  if (!parse_optional_int(b->year, 0, 3000, &year)) {
    snprintf(errbuf, errlen, "invalid year");
    return false;
  }
  if (!valid_date(b->publication_date)) {
    snprintf(errbuf, errlen, "invalid publication_date (expected YYYY-MM-DD)");
    return false;
  }
  return true;
}

/* The database generates PK ids; this is the only place that's assumed. */
#define BOOK_INSERT_SQL                                                    \
  "INSERT INTO public.books"                                               \
  " (id, title, author, isbn, category, publisher, publication_date,"      \
  "  description, cover, total_copies, available_copies, year, branch_id," \
  "  google_metadata)"                                                     \
  " VALUES (gen_random_uuid()::text, $1, $2, $3, $4, $5, $6, $7, $8, $9,"  \
  " $10, $11, $12, $13)"                                                   \
  " RETURNING id"

BookCreateStatus book_service_create(Database *db, const BookMutate *book, char *id_out,
                                     size_t id_size, char *errbuf, size_t errlen) {
  if (id_out == NULL || id_size == 0) {
    return BOOK_CREATE_BAD_INPUT;
  }
  if (!validate_book(book, true, errbuf, errlen)) {
    return BOOK_CREATE_BAD_INPUT;
  }
  if (!db_is_configured(db)) {
    return BOOK_CREATE_DB_UNAVAILABLE;
  }

  /* Empty optional strings fall back to the schema defaults; an empty
   * publication_date / branch_id is passed as SQL NULL. */
  const char *params[13];
  params[0] = book->title;
  params[1] = book->author[0] ? book->author : "";
  params[2] = book->isbn;
  params[3] = book->category[0] ? book->category : "";
  params[4] = book->publisher[0] ? book->publisher : "";
  params[5] = book->publication_date[0] ? book->publication_date : NULL;
  params[6] = book->description[0] ? book->description : "";
  params[7] = book->cover[0] ? book->cover : "";
  params[8] = book->total_copies[0] ? book->total_copies : "1";
  params[9] = book->available_copies[0] ? book->available_copies : "1";
  params[10] = book->year[0] ? book->year : "0";
  params[11] = book->branch_id[0] ? book->branch_id : NULL;
  params[12] = book->google_metadata[0] ? book->google_metadata : "{}";

  DbResult result;
  if (!db_query(db, BOOK_INSERT_SQL, params, 13, &result, errbuf, errlen)) {
    if (db_has_sqlstate(errbuf, DB_SQLSTATE_UNIQUE_VIOLATION)) {
      return BOOK_CREATE_DUPLICATE;
    }
    return BOOK_CREATE_QUERY_FAILED;
  }

  BookCreateStatus status = BOOK_CREATE_QUERY_FAILED;
  if (result.nrows == 1 && result.values != NULL && result.values[0] != NULL) {
    snprintf(id_out, id_size, "%s", result.values[0]);
    status = BOOK_CREATE_OK;
  }
  db_result_free(&result);
  return status;
}

/*
 * Update only the columns whose BookMutate field is non-empty. An empty
 * string therefore never clears a column - that is a documented limitation.
 */
BookUpdateStatus book_service_update(Database *db, const char *id, const BookMutate *book,
                                     char *errbuf, size_t errlen) {
  if (id == NULL || id[0] == '\0') {
    return BOOK_UPDATE_NOT_FOUND;
  }
  if (!validate_book(book, false, errbuf, errlen)) {
    return BOOK_UPDATE_BAD_INPUT;
  }
  if (!db_is_configured(db)) {
    return BOOK_UPDATE_DB_UNAVAILABLE;
  }

  char sql[1024];
  snprintf(sql, sizeof(sql), "UPDATE public.books SET");

  const char *params[13];
  int nparams = 0;

#define ADD_SET(col, field)                                                 \
  if ((field)[0] != '\0') {                                                 \
    strncat(sql, (nparams == 0) ? "" : ", ", sizeof(sql) - strlen(sql) - 1); \
    char seg[64];                                                          \
    snprintf(seg, sizeof(seg), " %s = $%d", col, nparams + 1);             \
    strncat(sql, seg, sizeof(sql) - strlen(sql) - 1);                      \
    params[nparams++] = (field);                                           \
  }

  ADD_SET("title", book->title);
  ADD_SET("author", book->author);
  ADD_SET("isbn", book->isbn);
  ADD_SET("category", book->category);
  ADD_SET("publisher", book->publisher);
  ADD_SET("publication_date", book->publication_date);
  ADD_SET("description", book->description);
  ADD_SET("cover", book->cover);
  ADD_SET("branch_id", book->branch_id);
  ADD_SET("total_copies", book->total_copies);
  ADD_SET("available_copies", book->available_copies);
  ADD_SET("year", book->year);

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
      return BOOK_UPDATE_DUPLICATE;
    }
    return BOOK_UPDATE_QUERY_FAILED;
  }

  BookUpdateStatus status = BOOK_UPDATE_NOT_FOUND;
  if (result.nrows == 1) {
    status = BOOK_UPDATE_OK;
  }
  db_result_free(&result);
  return status;
}

BookDeleteStatus book_service_delete(Database *db, const char *id, char *errbuf,
                                      size_t errlen) {
  if (id == NULL || id[0] == '\0') {
    return BOOK_DELETE_NOT_FOUND;
  }
  if (!db_is_configured(db)) {
    return BOOK_DELETE_DB_UNAVAILABLE;
  }

  const char *params[1] = {id};
  DbResult result;
  if (!db_query(db, "DELETE FROM public.books WHERE id = $1 RETURNING id", params, 1,
                &result, errbuf, errlen)) {
    if (db_has_sqlstate(errbuf, DB_SQLSTATE_FOREIGN_KEY_VIOLATION)) {
      return BOOK_DELETE_IN_USE;
    }
    return BOOK_DELETE_QUERY_FAILED;
  }

  BookDeleteStatus status = BOOK_DELETE_NOT_FOUND;
  if (result.nrows == 1) {
    status = BOOK_DELETE_OK;
  }
  db_result_free(&result);
  return status;
}

static void append_import_code(char *results, size_t cap, const char *code) {
  size_t len = strlen(results);
  int written;
  if (len == 0) {
    written = snprintf(results + len, cap - len, "\"%s\"", code);
  } else {
    written = snprintf(results + len, cap - len, ",\"%s\"", code);
  }
  (void)written;
}

BookImportStatus book_service_import_bulk(Database *db, const char *json, size_t json_len,
                                           char *out, size_t out_size, char *errbuf,
                                           size_t errlen) {
  (void)errbuf;
  (void)errlen;
  if (out == NULL || out_size == 0) {
    return BOOK_IMPORT_BUFFER_TOO_SMALL;
  }
  if (json == NULL || json_len == 0 || json_len > BOOKS_IMPORT_JSON_MAX) {
    return BOOK_IMPORT_BUFFER_TOO_SMALL;
  }
  if (!db_is_configured(db)) {
    return BOOK_IMPORT_DB_UNAVAILABLE;
  }

  const char *array_key = "books";
  int book_count = json_array_count(json, array_key);
  if (book_count > BOOKS_IMPORT_MAX) {
    if (errbuf != NULL && errlen > 0) {
      snprintf(errbuf, errlen, "too many books: %d (max %d)", book_count, BOOKS_IMPORT_MAX);
    }
    return BOOK_IMPORT_TOO_MANY;
  }
  if (book_count <= 0) {
    snprintf(out, out_size,
             "{\"created\":0,\"duplicates\":0,\"failed\":0,\"results\":[]}");
    return BOOK_IMPORT_OK;
  }

  int created = 0, duplicates = 0, failed = 0;
  char results[12288] = {0};

  for (int i = 0; i < book_count; i++) {
    char book_json[131072];
    int n = json_array_get(json, array_key, i, book_json, sizeof(book_json));
    if (n <= 0) {
      failed++;
      append_import_code(results, sizeof(results), "failed");
      continue;
    }

    BookMutate book;
    memset(&book, 0, sizeof(book));

    char title[512] = {0}, author[512] = {0}, isbn[512] = {0};
    char category[512] = {0}, publisher[512] = {0};
    char pub_date[64] = {0}, desc[2048] = {0}, cover[2048] = {0};
    char copies[32] = {0}, avail[32] = {0}, year[16] = {0}, branch[256] = {0};
    char metadata[BOOK_METADATA_MAX] = {0};

    json_string_of_key(book_json, "title", title, sizeof(title));
    json_string_of_key(book_json, "author", author, sizeof(author));
    json_string_of_key(book_json, "isbn", isbn, sizeof(isbn));
    json_string_of_key(book_json, "category", category, sizeof(category));
    json_string_of_key(book_json, "publisher", publisher, sizeof(publisher));
    json_string_of_key(book_json, "publicationDate", pub_date, sizeof(pub_date));
    json_string_of_key(book_json, "description", desc, sizeof(desc));
    json_string_of_key(book_json, "cover", cover, sizeof(cover));
    json_string_of_key(book_json, "totalCopies", copies, sizeof(copies));
    if (copies[0] == '\0') {
      json_string_of_key(book_json, "copies", copies, sizeof(copies));
    }
    json_string_of_key(book_json, "availableCopies", avail, sizeof(avail));
    if (avail[0] == '\0') {
      snprintf(avail, sizeof(avail), "%s", copies);
    }
    json_string_of_key(book_json, "year", year, sizeof(year));
    json_string_of_key(book_json, "branchId", branch, sizeof(branch));
    json_string_of_key(book_json, "googleMetadata", metadata, sizeof(metadata));
    normalize_partial_date(pub_date, sizeof(pub_date));

    snprintf(book.google_metadata, sizeof(book.google_metadata), "%s", metadata);
    snprintf(book.title, sizeof(book.title), "%s", title);
    snprintf(book.author, sizeof(book.author), "%s", author);
    snprintf(book.isbn, sizeof(book.isbn), "%s", isbn);
    snprintf(book.category, sizeof(book.category), "%s", category);
    snprintf(book.publisher, sizeof(book.publisher), "%s", publisher);
    snprintf(book.publication_date, sizeof(book.publication_date), "%s", pub_date);
    snprintf(book.description, sizeof(book.description), "%s", desc);
    snprintf(book.cover, sizeof(book.cover), "%s", cover);
    snprintf(book.total_copies, sizeof(book.total_copies), "%s", copies);
    snprintf(book.available_copies, sizeof(book.available_copies), "%s", avail);
    snprintf(book.year, sizeof(book.year), "%s", year);
    snprintf(book.branch_id, sizeof(book.branch_id), "%s", branch);

    char idbuf[128];
    char single_err[256] = {0};
    BookCreateStatus cs = book_service_create(db, &book, idbuf, sizeof(idbuf),
                                               single_err, sizeof(single_err));
    switch (cs) {
      case BOOK_CREATE_OK:
        created++;
        append_import_code(results, sizeof(results), "ok");
        break;
      case BOOK_CREATE_DUPLICATE:
        duplicates++;
        append_import_code(results, sizeof(results), "duplicate");
        break;
      default:
        fprintf(stderr, "[books] bulk row %d failed: %s\n", i + 1, single_err);
        failed++;
        append_import_code(results, sizeof(results), "failed");
        break;
    }
  }

  int written = snprintf(out, out_size,
    "{\"created\":%d,\"duplicates\":%d,\"failed\":%d,\"results\":[%s]}",
    created, duplicates, failed, results);
  if (written < 0 || (size_t)written >= out_size) {
    return BOOK_IMPORT_BUFFER_TOO_SMALL;
  }
  return BOOK_IMPORT_OK;
}
