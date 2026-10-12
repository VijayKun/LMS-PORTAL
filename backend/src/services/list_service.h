/**
 * list_service.h - Shared engine for read-only list + get-by-id endpoints.
 *
 * Loans, reservations, fines and notifications all follow the same shape:
 * `{"<key>":[...],"limit":L,"offset":O}` for a list and `{"<key>":{...}}` for a
 * single row. This tiny engine owns that SQL + JSON so each resource only has
 * to declare *which* columns, filters and order it exposes. Column names and
 * tables live in the (compiled-in) ListSpec, never in HTTP input, so no user
 * value can add SQL.
 */
#ifndef LMS_LIST_SERVICE_H
#define LMS_LIST_SERVICE_H

#include <stddef.h>

#include "database/db.h"

typedef enum {
  LIST_OK = 0,
  LIST_DB_UNAVAILABLE,  /* no database configured/connected */
  LIST_QUERY_FAILED,    /* the query itself failed */
  LIST_NOT_FOUND,       /* no row matched the primary key */
  LIST_BUFFER_TOO_SMALL /* result did not fit the output buffer */
} ListStatus;

/** How many filters a list endpoint may expose (bounds the WHERE clause). */
#define LIST_MAX_FILTERS 8

/**
 * A read-only resource definition. All strings are compile-time constants.
 * filter_columns _only_ lists exact-match columns the API may filter on; the
 * filter *values* themselves arrive as query parameters and are bound in SQL.
 */
typedef struct {
  const char *response_key;              /* JSON key, e.g. "loans" */
  const char *columns;                   /* comma-separated SELECT list */
  const char *table;                     /* e.g. "public.transactions" */
  const char *order_by;                  /* column to ORDER BY */
  const char *filter_columns[LIST_MAX_FILTERS]; /* NULL-terminated */
} ListSpec;

/** Paging + filter values for a list query, parallel to the spec's filters. */
typedef struct {
  const char *filter_values[LIST_MAX_FILTERS]; /* may be NULL/"" for "not set" */
  int limit;                                  /* <= 0 -> default */
  int offset;                                 /* < 0 -> 0 */
} ListQuery;

/** Default and maximum page size shared by the list endpoints. */
#define LIST_DEFAULT_LIMIT 100
#define LIST_MAX_LIMIT 500

/**
 * Load a page from the resource defined by spec, ordered by spec->order_by.
 *
 * On LIST_OK the output holds
 * {"<response_key>":[...],"limit":L,"offset":O}. Only filter columns listed in
 * spec->filter_columns are applied; everything else is ignored.
 *
 * @param db       database handle
 * @param spec     compiled-in resource definition
 * @param query    paging + filter values (may be NULL for defaults)
 * @param out      output buffer
 * @param out_size size of out
 * @param errbuf   receives server-side error detail (log it, don't send it)
 * @param errlen   size of errbuf
 */
ListStatus list_service_list(Database *db, const ListSpec *spec, const ListQuery *query,
                             char *out, size_t out_size, char *errbuf, size_t errlen);

/**
 * Load a single row by the given primary-key column.
 *
 * On LIST_OK the output holds {"<response_key>":{...}}. When no row matches the
 * pk_value the engine reports LIST_NOT_FOUND so the caller can answer 404.
 */
ListStatus list_service_get(Database *db, const char *table, const char *pk,
                            const char *pk_value, const char *columns,
                            const char *response_key, char *out, size_t out_size,
                            char *errbuf, size_t errlen);

#endif /* LMS_LIST_SERVICE_H */