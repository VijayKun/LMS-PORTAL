/**
 * db.h - PostgreSQL (Supabase) connection and query boundary.
 *
 * All libpq usage is confined to db.c; the rest of the code base only sees the
 * small, driver-agnostic types below. That keeps SQL details out of controllers
 * and services.
 *
 * SECURITY: credentials come from configuration (environment variables / .env),
 * never from source. Error buffers may contain server-side text; callers must
 * log them, not return them verbatim to HTTP clients.
 */
#ifndef LMS_DB_H
#define LMS_DB_H

#include <stdbool.h>
#include <stddef.h>

#include "config/config.h"

/*
 * Forward declaration matching libpq's own typedef, so this header does not
 * need to include libpq-fe.h. (Identical re-typedef of the same struct is
 * allowed by C11.)
 */
typedef struct pg_conn PGconn;

/** Runtime database handle. */
typedef struct {
  PGconn *conn;
  bool connected;
} Database;

/**
 * A materialised query result.
 *
 * values is row-major: values[row * ncols + col]; a NULL entry means SQL NULL.
 * All strings are owned by the result and freed by db_result_free().
 */
typedef struct {
  int nrows;
  int ncols;
  char **columns; /* ncols column names */
  int *kinds;     /* ncols JSON value hints (see JSON_VALUE_* in json_utils.h) */
  char **values;  /* nrows * ncols values, row-major */
} DbResult;

/**
 * Open a connection using configuration. Safe to call with an incomplete
 * config: sets connected = false and logs instead of crashing.
 */
void db_init(Database *db, const DbConfig *cfg);

/** Close the connection (no-op if already closed). */
void db_close(Database *db);

/** @return true when a live connection is available. */
bool db_is_configured(const Database *db);

/**
 * Execute a parameterised query (text-format parameters only).
 *
 * @param db      connected database
 * @param sql     SQL with $1, $2 ... placeholders
 * @param params  parameter text values (may be NULL when nparams == 0)
 * @param nparams number of parameters
 * @param out     result sink; zeroed on failure
 * @param errbuf  buffer that receives a server-side error description
 * @param errlen  size of errbuf
 * @return true on success (call db_result_free), false on failure
 */
bool db_query(Database *db, const char *sql, const char *const *params, int nparams,
              DbResult *out, char *errbuf, size_t errlen);

/** Free everything owned by a DbResult and reset it. */
void db_result_free(DbResult *result);

#endif /* LMS_DB_H */
