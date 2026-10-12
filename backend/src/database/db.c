/**
 * db.c - libpq-backed implementation of the database boundary (see db.h).
 *
 * Every libpq call in the project lives in this file. Connection errors are
 * logged to stderr only and never returned to HTTP clients.
 */
#include "database/db.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libpq-fe.h>

#include "utils/json_utils.h" /* JSON_VALUE_* kind hints */

/** strdup replacement (strdup is POSIX; we target strict C11). */
static char *dup_string(const char *s) {
  if (s == NULL) {
    return NULL;
  }
  size_t len = strlen(s) + 1;
  char *copy = malloc(len);
  if (copy != NULL) {
    memcpy(copy, s, len);
  }
  return copy;
}

static void set_error(char *buf, size_t len, const char *msg) {
  if (buf == NULL || len == 0) {
    return;
  }
  snprintf(buf, len, "%s", (msg != NULL) ? msg : "unknown error");
}

/*
 * Map a PostgreSQL type OID to the JSON representation we should emit.
 * Numeric OIDs: bool=16, bytea? no; int8=20, int2=21, int4=23, oid=26,
 * float4=700, float8=701, numeric=1700.
 */
static int kind_for_oid(Oid oid) {
  switch (oid) {
    case 16:   return JSON_VALUE_BOOL;
    case 20:   /* int8 */
    case 21:   /* int2 */
    case 23:   /* int4 */
    case 26:   /* oid */
    case 700:  /* float4 */
    case 701:  /* float8 */
    case 1700: /* numeric */
      return JSON_VALUE_NUMBER;
    default:
      return JSON_VALUE_STRING;
  }
}

void db_init(Database *db, const DbConfig *cfg) {
  if (db == NULL) {
    return;
  }

  db->conn = NULL;
  db->connected = false;

  if (cfg == NULL || cfg->host[0] == '\0') {
    printf("[db] not configured (LMS_DB_HOST is empty) - book endpoints will "
           "return 503.\n");
    return;
  }

  const char *keywords[] = {"host",         "port",    "dbname",
                            "user",         "password", "sslmode",
                            "connect_timeout", NULL};
  const char *values[] = {cfg->host, cfg->port,       cfg->name,
                          cfg->user, cfg->password,   cfg->sslmode,
                          "10",      NULL};

  PGconn *conn = PQconnectdbParams(keywords, values, 0);
  if (conn == NULL) {
    fprintf(stderr, "[db] connection failed: out of memory\n");
    return;
  }

  if (PQstatus(conn) != CONNECTION_OK) {
    /* Server-side log only. PQerrorMessage never contains the password. */
    fprintf(stderr, "[db] connection failed: %s", PQerrorMessage(conn));
    PQfinish(conn);
    return;
  }

  db->conn = conn;
  db->connected = true;
  printf("[db] connected to %s:%s/%s as %s (sslmode=%s)\n", cfg->host, cfg->port,
         cfg->name, cfg->user[0] ? cfg->user : "(default)", cfg->sslmode);
}

void db_close(Database *db) {
  if (db == NULL) {
    return;
  }
  if (db->conn != NULL) {
    PQfinish(db->conn);
  }
  db->conn = NULL;
  db->connected = false;
}

bool db_is_configured(const Database *db) {
  return db != NULL && db->connected && db->conn != NULL;
}

bool db_query(Database *db, const char *sql, const char *const *params, int nparams,
              DbResult *out, char *errbuf, size_t errlen) {
  if (out != NULL) {
    out->nrows = 0;
    out->ncols = 0;
    out->columns = NULL;
    out->kinds = NULL;
    out->values = NULL;
  }

  if (!db_is_configured(db)) {
    set_error(errbuf, errlen, "database is not connected");
    return false;
  }

  PGresult *res = PQexecParams(db->conn, sql, nparams, NULL, params, NULL, NULL, 0);
  if (res == NULL) {
    set_error(errbuf, errlen, "query could not be executed");
    return false;
  }

  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    /* Prefix the SQLSTATE so db_has_sqlstate() can classify the failure. */
    char combined[512];
    const char *sqlstate = PQresultErrorField(res, PG_DIAG_SQLSTATE);
    if (sqlstate != NULL && sqlstate[0] != '\0') {
      snprintf(combined, sizeof(combined), "[SQLSTATE %s] %s", sqlstate,
               PQresultErrorMessage(res));
    } else {
      snprintf(combined, sizeof(combined), "%s", PQresultErrorMessage(res));
    }
    set_error(errbuf, errlen, combined);
    PQclear(res);
    return false;
  }

  if (out == NULL) {
    PQclear(res);
    return true;
  }

  int nrows = PQntuples(res);
  int ncols = PQnfields(res);

  out->columns = (ncols > 0) ? calloc((size_t)ncols, sizeof(char *)) : NULL;
  out->kinds = (ncols > 0) ? calloc((size_t)ncols, sizeof(int)) : NULL;
  out->values =
      (nrows > 0 && ncols > 0) ? calloc((size_t)nrows * (size_t)ncols, sizeof(char *)) : NULL;

  if ((ncols > 0 && (out->columns == NULL || out->kinds == NULL)) ||
      (nrows > 0 && ncols > 0 && out->values == NULL)) {
    set_error(errbuf, errlen, "out of memory while reading the result");
    PQclear(res);
    db_result_free(out);
    return false;
  }

  out->nrows = nrows;
  out->ncols = ncols;

  for (int col = 0; col < ncols; col++) {
    out->columns[col] = dup_string(PQfname(res, col));
    out->kinds[col] = kind_for_oid(PQftype(res, col));
  }

  for (int row = 0; row < nrows; row++) {
    for (int col = 0; col < ncols; col++) {
      int index = row * ncols + col;
      if (PQgetisnull(res, row, col)) {
        out->values[index] = NULL;
      } else {
        out->values[index] = dup_string(PQgetvalue(res, row, col));
      }
    }
  }

  PQclear(res);
  return true;
}

bool db_has_sqlstate(const char *errbuf, const char *state) {
  if (errbuf == NULL || state == NULL || state[0] == '\0') {
    return false;
  }
  char needle[96];
  snprintf(needle, sizeof(needle), "[SQLSTATE %s]", state);
  return strstr(errbuf, needle) != NULL;
}

bool db_execute(Database *db, const char *sql, const char *const *params, int nparams,
                char *errbuf, size_t errlen) {
  if (!db_is_configured(db)) {
    set_error(errbuf, errlen, "database is not connected");
    return false;
  }

  PGresult *res = PQexecParams(db->conn, sql, nparams, NULL, params, NULL, NULL, 0);
  if (res == NULL) {
    set_error(errbuf, errlen, "query could not be executed");
    return false;
  }

  ExecStatusType status = PQresultStatus(res);
  if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK) {
    char combined[512];
    const char *sqlstate = PQresultErrorField(res, PG_DIAG_SQLSTATE);
    if (sqlstate != NULL && sqlstate[0] != '\0') {
      snprintf(combined, sizeof(combined), "[SQLSTATE %s] %s", sqlstate,
               PQresultErrorMessage(res));
    } else {
      snprintf(combined, sizeof(combined), "%s", PQresultErrorMessage(res));
    }
    set_error(errbuf, errlen, combined);
    PQclear(res);
    return false;
  }

  PQclear(res);
  return true;
}

void db_result_free(DbResult *result) {
  if (result == NULL) {
    return;
  }

  if (result->columns != NULL) {
    for (int col = 0; col < result->ncols; col++) {
      free(result->columns[col]);
    }
    free(result->columns);
  }

  if (result->values != NULL) {
    int total = result->nrows * result->ncols;
    for (int i = 0; i < total; i++) {
      free(result->values[i]);
    }
    free(result->values);
  }

  free(result->kinds);

  result->nrows = 0;
  result->ncols = 0;
  result->columns = NULL;
  result->kinds = NULL;
  result->values = NULL;
}
