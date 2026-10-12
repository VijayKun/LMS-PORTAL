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

typedef enum {
  BOOK_LIST_OK = 0,
  BOOK_LIST_DB_UNAVAILABLE,  /* no database configured/connected */
  BOOK_LIST_QUERY_FAILED,    /* the query itself failed */
  BOOK_LIST_BUFFER_TOO_SMALL /* result did not fit the output buffer */
} BookListStatus;

/**
 * Load every book from public.books, ordered by title, as JSON.
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

#endif /* LMS_BOOK_SERVICE_H */
