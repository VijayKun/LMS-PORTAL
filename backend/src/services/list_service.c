/**
 * list_service.c - Generic read-only list + get-by-id engine.
 *
 * All identifiers (table, columns, order, filter columns) come from the
 * compile-time ListSpec; only filter *values*, limit and offset arrive from the
 * client, and those are bound as query parameters. Like the other services,
 * database errors are logged by the caller and never shipped to the client.
 */
#include "services/list_service.h"

#include <stdio.h>
#include <string.h>

#include "utils/json_utils.h"

ListStatus list_service_list(Database *db, const ListSpec *spec, const ListQuery *query,
                             char *out, size_t out_size, char *errbuf, size_t errlen) {
  if (out == NULL || out_size == 0) {
    return LIST_BUFFER_TOO_SMALL;
  }
  if (spec == NULL || spec->table == NULL || spec->columns == NULL ||
      spec->response_key == NULL) {
    return LIST_BUFFER_TOO_SMALL; /* programmer error, not a client issue */
  }
  if (!db_is_configured(db)) {
    return LIST_DB_UNAVAILABLE;
  }

  /* Page size, clamped like the book catalog. */
  int limit = (query != NULL && query->limit > 0) ? query->limit : LIST_DEFAULT_LIMIT;
  if (limit > LIST_MAX_LIMIT) {
    limit = LIST_MAX_LIMIT;
  }
  int offset = (query != NULL && query->offset > 0) ? query->offset : 0;

  const char *params[LIST_MAX_FILTERS + 2];
  char limit_buf[16];
  char offset_buf[16];
  char where[256] = "";
  int nparams = 0;

  for (int i = 0; i < LIST_MAX_FILTERS && spec->filter_columns[i] != NULL; i++) {
    const char *value = (query != NULL) ? query->filter_values[i] : NULL;
    if (value == NULL || value[0] == '\0') {
      continue;
    }
    nparams++;
    params[nparams - 1] = value;
    size_t wlen = strlen(where);
    snprintf(where + wlen, sizeof(where) - wlen, "%s %s = $%d",
             (wlen == 0) ? " WHERE" : " AND", spec->filter_columns[i], nparams);
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
           "SELECT %s FROM %s%s ORDER BY %s LIMIT $%d OFFSET $%d", spec->columns,
           spec->table, where, spec->order_by, limit_ph, offset_ph);

  DbResult result;
  if (!db_query(db, sql, params, nparams, &result, errbuf, errlen)) {
    return LIST_QUERY_FAILED;
  }

  char suffix[64];
  int suffix_len = snprintf(suffix, sizeof(suffix), ",\"limit\":%d,\"offset\":%d}", limit,
                            offset);

  char prefix[64];
  int prefix_len = snprintf(prefix, sizeof(prefix), "{\"%s\":", spec->response_key);
  if (prefix_len < 0 || prefix_len + 3 > (int)out_size) {
    db_result_free(&result);
    return LIST_BUFFER_TOO_SMALL;
  }
  memcpy(out, prefix, (size_t)prefix_len);

  int written = json_array_of_objects((const char *const *)result.columns, result.kinds,
                                      (const char *const *)result.values, result.nrows,
                                      result.ncols, out + prefix_len, out_size - prefix_len);
  db_result_free(&result);

  if (written < 0) {
    return LIST_BUFFER_TOO_SMALL;
  }

  size_t total = (size_t)prefix_len + (size_t)written;
  if (total + (size_t)suffix_len + 1 > out_size) {
    return LIST_BUFFER_TOO_SMALL;
  }

  memcpy(out + total, suffix, (size_t)suffix_len);
  out[total + (size_t)suffix_len] = '\0';
  return LIST_OK;
}

ListStatus list_service_get(Database *db, const char *table, const char *pk,
                            const char *pk_value, const char *columns,
                            const char *response_key, char *out, size_t out_size,
                            char *errbuf, size_t errlen) {
  if (out == NULL || out_size == 0) {
    return LIST_BUFFER_TOO_SMALL;
  }
  if (table == NULL || pk == NULL || columns == NULL || response_key == NULL) {
    return LIST_BUFFER_TOO_SMALL; /* programmer error */
  }
  if (pk_value == NULL || pk_value[0] == '\0') {
    return LIST_NOT_FOUND;
  }
  if (!db_is_configured(db)) {
    return LIST_DB_UNAVAILABLE;
  }

  char sql[192];
  snprintf(sql, sizeof(sql), "SELECT %s FROM %s WHERE %s = $1", columns, table, pk);

  const char *params[1] = {pk_value};
  DbResult result;
  if (!db_query(db, sql, params, 1, &result, errbuf, errlen)) {
    return LIST_QUERY_FAILED;
  }

  if (result.nrows == 0) {
    db_result_free(&result);
    return LIST_NOT_FOUND;
  }

  char prefix[64];
  int prefix_len = snprintf(prefix, sizeof(prefix), "{\"%s\":", response_key);
  if (prefix_len < 0 || prefix_len + 3 > (int)out_size) {
    db_result_free(&result);
    return LIST_BUFFER_TOO_SMALL;
  }
  memcpy(out, prefix, (size_t)prefix_len);

  int written = json_object_of_row((const char *const *)result.columns, result.kinds,
                                   (const char *const *)result.values, result.ncols,
                                   out + prefix_len, out_size - prefix_len);
  db_result_free(&result);

  if (written < 0) {
    return LIST_BUFFER_TOO_SMALL;
  }

  size_t total = (size_t)prefix_len + (size_t)written;
  if (total + 2 > out_size) {
    return LIST_BUFFER_TOO_SMALL;
  }

  out[total] = '}';
  out[total + 1] = '\0';
  return LIST_OK;
}