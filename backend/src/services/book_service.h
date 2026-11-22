/**
 * book_service.h - Book catalog business logic.
 *
 * Owns the SQL and turns database rows into JSON. Controllers stay thin and
 * never touch libpq or column names.
 */
#ifndef LMS_BOOK_SERVICE_H
#define LMS_BOOK_SERVICE_H

#include <stddef.h>

#include "database/db.h"

/** Maximum length of any single editable book field. */
#define BOOK_FIELD_MAX 256

/**
 * Editable fields used by create/update. Everything stays text so JSON parsing
 * and validation live in the service. An empty string means "not provided":
 * create falls back to schema defaults, update skips the column entirely.
 * `title` and `isbn` are required on create (and in updates that include them).
 */
typedef struct {
  char title[BOOK_FIELD_MAX];
  char author[BOOK_FIELD_MAX];
  char isbn[BOOK_FIELD_MAX];
  char category[BOOK_FIELD_MAX];
  char publisher[BOOK_FIELD_MAX];
  char publication_date[BOOK_FIELD_MAX]; /* YYYY-MM-DD or empty */
  char description[BOOK_FIELD_MAX];
  char cover[BOOK_FIELD_MAX];
  char branch_id[BOOK_FIELD_MAX];
  char total_copies[BOOK_FIELD_MAX];     /* non-negative integer, as text */
  char available_copies[BOOK_FIELD_MAX]; /* non-negative integer, as text */
  char year[BOOK_FIELD_MAX];             /* year (0..3000), as text */
} BookMutate;

typedef enum {
  BOOK_LIST_OK = 0,
  BOOK_LIST_DB_UNAVAILABLE,  /* no database configured/connected */
  BOOK_LIST_QUERY_FAILED,    /* the query itself failed */
  BOOK_LIST_BUFFER_TOO_SMALL /* result did not fit the output buffer */
} BookListStatus;

typedef enum {
  BOOK_GET_OK = 0,
  BOOK_GET_NOT_FOUND,        /* no book with the requested id */
  BOOK_GET_DB_UNAVAILABLE,   /* no database configured/connected */
  BOOK_GET_QUERY_FAILED,     /* the query itself failed */
  BOOK_GET_BUFFER_TOO_SMALL  /* result did not fit the output buffer */
} BookGetStatus;

/** Default and maximum page size for book searches. */
#define BOOKS_DEFAULT_LIMIT 100
#define BOOKS_MAX_LIMIT 500

/**
 * Optional filters for a catalog search. All fields are optional; a zeroed
 * struct (or NULL) means "first page, no filters".
 */
typedef struct {
  const char *q;        /* case-insensitive match on title OR author */
  const char *category; /* exact category match */
  int limit;            /* rows per page; <= 0 -> BOOKS_DEFAULT_LIMIT */
  int offset;           /* rows to skip; < 0 -> 0 */
} BookListQuery;

/**
 * Load every book from public.books, ordered by title, as JSON.
 * Equivalent to book_service_search() with no filters.
 *
 * On BOOK_LIST_OK the output buffer holds {"books":[...]}.
 *
 * @param db       database handle
 * @param out      output buffer
 * @param out_size size of out
 * @param errbuf   receives server-side error detail (log it, don't send it)
 * @param errlen   size of errbuf
 */
BookListStatus book_service_list(Database *db, char *out, size_t out_size, char *errbuf,
                                 size_t errlen);

/**
 * Load a page of the catalog with optional filters, ordered by title.
 *
 * On success the output holds {"books":[...],"limit":L,"offset":O}. Values are
 * bound as query parameters, so filters cannot inject SQL.
 *
 * @param db       database handle
 * @param query    filters and paging (may be NULL for defaults)
 * @param out      output buffer
 * @param out_size size of out
 * @param errbuf   receives server-side error detail (log it, don't send it)
 * @param errlen   size of errbuf
 */
BookListStatus book_service_search(Database *db, const BookListQuery *query, char *out,
                                   size_t out_size, char *errbuf, size_t errlen);

/**
 * Load a single book by id as JSON.
 *
 * On BOOK_GET_OK the output buffer holds {"book":{...}}. A missing id is
 * reported as BOOK_GET_NOT_FOUND (not an error).
 *
 * @param db       database handle
 * @param id       book id (must not be NULL or empty)
 * @param out      output buffer
 * @param out_size size of out
 * @param errbuf   receives server-side error detail (log it, don't send it)
 * @param errlen   size of errbuf
 */
BookGetStatus book_service_get(Database *db, const char *id, char *out, size_t out_size,
                               char *errbuf, size_t errlen);

/**
 * Load a single book by ISBN as JSON.
 *
 * On BOOK_GET_OK the output buffer holds {"book":{...}}. A missing ISBN is
 * reported as BOOK_GET_NOT_FOUND (not an error).
 *
 * @param db       database handle
 * @param isbn     ISBN (must not be NULL or empty)
 * @param out      output buffer
 * @param out_size size of out
 * @param errbuf   receives server-side error detail (log it, don't send it)
 * @param errlen   size of errbuf
 */
BookGetStatus book_service_get_by_isbn(Database *db, const char *isbn, char *out,
                                       size_t out_size, char *errbuf, size_t errlen);

typedef enum {
  BOOK_CREATE_OK = 0,
  BOOK_CREATE_BAD_INPUT,        /* title/isbn missing or a value is invalid */
  BOOK_CREATE_DUPLICATE,        /* isbn already exists (UNIQUE violation) */
  BOOK_CREATE_DB_UNAVAILABLE,   /* no database configured/connected */
  BOOK_CREATE_QUERY_FAILED      /* the query itself failed */
} BookCreateStatus;

/**
 * Insert a new book. The id is generated by the database
 * (gen_random_uuid()::text), so nothing here depends on pgcrypto.
 *
 * On BOOK_CREATE_OK, id_out receives the new row id so the caller can load the
 * full book with book_service_get().
 */
BookCreateStatus book_service_create(Database *db, const BookMutate *book, char *id_out,
                                     size_t id_size, char *errbuf, size_t errlen);

typedef enum {
  BOOK_UPDATE_OK = 0,
  BOOK_UPDATE_NOT_FOUND,       /* no book with the requested id */
  BOOK_UPDATE_BAD_INPUT,       /* no editable fields or a value is invalid */
  BOOK_UPDATE_DUPLICATE,       /* isbn already exists (UNIQUE violation) */
  BOOK_UPDATE_DB_UNAVAILABLE,  /* no database configured/connected */
  BOOK_UPDATE_QUERY_FAILED     /* the query itself failed */
} BookUpdateStatus;

/**
 * Update only the provided fields of a book. Same validation rules as create;
 * at least one field must be non-empty. Use PUT or PATCH semantics at the
 * route - the service is intentionally symmetric.
 */
BookUpdateStatus book_service_update(Database *db, const char *id, const BookMutate *book,
                                     char *errbuf, size_t errlen);

typedef enum {
  BOOK_DELETE_OK = 0,
  BOOK_DELETE_NOT_FOUND,       /* no book with the requested id */
  BOOK_DELETE_IN_USE,          /* referenced by transactions (FK RESTRICT) */
  BOOK_DELETE_DB_UNAVAILABLE,  /* no database configured/connected */
  BOOK_DELETE_QUERY_FAILED     /* the query itself failed */
} BookDeleteStatus;

/** Delete a book by id. Returns BOOK_DELETE_IN_USE when loans reference it. */
BookDeleteStatus book_service_delete(Database *db, const char *id, char *errbuf,
                                     size_t errlen);

typedef enum {
  BOOK_IMPORT_OK = 0,
  BOOK_IMPORT_DB_UNAVAILABLE,
  BOOK_IMPORT_QUERY_FAILED,
  BOOK_IMPORT_BUFFER_TOO_SMALL
} BookImportStatus;

#define BOOKS_IMPORT_MAX 500
#define BOOKS_IMPORT_JSON_MAX 1048576

BookImportStatus book_service_import_bulk(Database *db, const char *json, size_t json_len,
                                          char *out, size_t out_size, char *errbuf,
                                          size_t errlen);

#endif /* LMS_BOOK_SERVICE_H */
