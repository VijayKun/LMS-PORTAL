/**
 * notification_service.c - Notifications lists and read/announcement writes.
 */
#include "services/notification_service.h"

#include <stdio.h>
#include <string.h>

#include "utils/json_utils.h"

static const char *NOTIFICATION_COLUMNS = "id, type, title, message, time, read, user_id";

ListStatus notification_service_list(Database *db, const NotificationListQuery *query,
                                     char *out, size_t out_size, char *errbuf,
                                     size_t errlen) {
  if (out == NULL || out_size == 0) {
    return LIST_BUFFER_TOO_SMALL;
  }
  if (!db_is_configured(db)) {
    return LIST_DB_UNAVAILABLE;
  }

  int limit = (query != NULL && query->limit > 0) ? query->limit : LIST_DEFAULT_LIMIT;
  if (limit > LIST_MAX_LIMIT) {
    limit = LIST_MAX_LIMIT;
  }
  int offset = (query != NULL && query->offset > 0) ? query->offset : 0;

  const char *params[4];
  char limit_buf[16];
  char offset_buf[16];
  int nparams = 0;
  char where[192] = "";

  if (query != NULL && query->user_id != NULL && query->user_id[0] != '\0') {
    params[nparams++] = query->user_id;
    snprintf(where, sizeof(where), " WHERE (user_id = $%d OR user_id = 'all')", nparams);
  }
  if (query != NULL && query->type != NULL && query->type[0] != '\0') {
    params[nparams++] = query->type;
    snprintf(where + strlen(where), sizeof(where) - strlen(where), "%s type = $%d",
             where[0] ? " AND" : " WHERE", nparams);
  }

  snprintf(limit_buf, sizeof(limit_buf), "%d", limit);
  params[nparams++] = limit_buf;
  int limit_ph = nparams;
  snprintf(offset_buf, sizeof(offset_buf), "%d", offset);
  params[nparams++] = offset_buf;
  int offset_ph = nparams;

  char sql[512];
  snprintf(sql, sizeof(sql),
           "SELECT %s FROM public.notifications%s ORDER BY time DESC, id "
           "LIMIT $%d OFFSET $%d",
           NOTIFICATION_COLUMNS, where, limit_ph, offset_ph);

  DbResult result;
  memset(&result, 0, sizeof(result));
  if (!db_query(db, sql, params, nparams, &result, errbuf, errlen)) {
    db_result_free(&result);
    return LIST_QUERY_FAILED;
  }

  char prefix[64];
  int prefix_len = snprintf(prefix, sizeof(prefix), "{\"notifications\":");
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

  char suffix[64];
  int suffix_len = snprintf(suffix, sizeof(suffix), ",\"limit\":%d,\"offset\":%d}", limit,
                            offset);
  size_t total = (size_t)prefix_len + (size_t)written;
  if (total + (size_t)suffix_len + 1 > out_size) {
    return LIST_BUFFER_TOO_SMALL;
  }
  memcpy(out + total, suffix, (size_t)suffix_len);
  out[total + (size_t)suffix_len] = '\0';
  return LIST_OK;
}

ListStatus notification_service_get(Database *db, const char *id, char *out, size_t out_size,
                                    char *errbuf, size_t errlen) {
  return list_service_get(db, "public.notifications", "id", id, NOTIFICATION_COLUMNS,
                          "notification", out, out_size, errbuf, errlen);
}

NotificationOwnerStatus notification_service_get_owner(Database *db, const char *id,
                                                       char *out, size_t out_size,
                                                       char *errbuf, size_t errlen) {
  if (id == NULL || id[0] == '\0' || out == NULL || out_size == 0) {
    return NOTIFICATION_OWNER_NOT_FOUND;
  }
  const char *params[1] = {id};
  DbResult result;
  memset(&result, 0, sizeof(result));
  if (!db_query(db, "SELECT user_id FROM public.notifications WHERE id = $1 LIMIT 1", params,
                1, &result, errbuf, errlen)) {
    db_result_free(&result);
    if (db == NULL || !db->connected) {
      return NOTIFICATION_OWNER_DB_UNAVAILABLE;
    }
    return NOTIFICATION_OWNER_QUERY_FAILED;
  }
  if (result.nrows == 0) {
    db_result_free(&result);
    return NOTIFICATION_OWNER_NOT_FOUND;
  }
  const char *owner = result.values[0];
  snprintf(out, out_size, "%s", owner == NULL ? "" : owner);
  db_result_free(&result);
  return NOTIFICATION_OWNER_OK;
}

NotificationCreateStatus notification_service_create(Database *db,
                                                      const NotificationInput *input,
                                                      char *id_out, size_t id_size,
                                                      char *errbuf, size_t errlen) {
  if (input == NULL || input->type[0] == '\0' || input->title[0] == '\0' ||
      id_out == NULL || id_size == 0) {
    return NOTIFICATION_CREATE_BAD_INPUT;
  }
  if (db == NULL || !db->connected) {
    return NOTIFICATION_CREATE_DB_UNAVAILABLE;
  }

  const char *user_id = input->user_id[0] ? input->user_id : "all";
  const char *params[4] = {input->type, input->title, input->message, user_id};
  DbResult result;
  memset(&result, 0, sizeof(result));

  const char *sql =
      "INSERT INTO public.notifications (id, type, title, message, user_id) "
      "VALUES (md5(random()::text || clock_timestamp()::text), $1, $2, $3, $4) "
      "RETURNING id";
  if (!db_query(db, sql, params, 4, &result, errbuf, errlen)) {
    db_result_free(&result);
    return NOTIFICATION_CREATE_QUERY_FAILED;
  }
  if (result.nrows == 0) {
    db_result_free(&result);
    return NOTIFICATION_CREATE_QUERY_FAILED;
  }
  snprintf(id_out, id_size, "%s", result.values[0] == NULL ? "" : result.values[0]);
  db_result_free(&result);
  return NOTIFICATION_CREATE_OK;
}

NotificationUpdateStatus notification_service_mark_read(Database *db, const char *id,
                                                        const char *scope_user,
                                                        char *errbuf, size_t errlen) {
  if (id == NULL || id[0] == '\0') {
    return NOTIFICATION_UPDATE_BAD_INPUT;
  }
  if (db == NULL || !db->connected) {
    return NOTIFICATION_UPDATE_DB_UNAVAILABLE;
  }

  const char *params[2] = {id, scope_user};
  const char *sql =
      scope_user == NULL
          ? "UPDATE public.notifications SET read = true WHERE id = $1 RETURNING id"
          : "UPDATE public.notifications SET read = true WHERE id = $1 "
            "AND (user_id = $2 OR user_id = 'all') RETURNING id";
  DbResult result;
  memset(&result, 0, sizeof(result));
  if (!db_query(db, sql, params, scope_user == NULL ? 1 : 2, &result, errbuf, errlen)) {
    db_result_free(&result);
    return NOTIFICATION_UPDATE_QUERY_FAILED;
  }
  NotificationUpdateStatus status =
      result.nrows == 0 ? NOTIFICATION_UPDATE_NOT_FOUND : NOTIFICATION_UPDATE_OK;
  db_result_free(&result);
  return status;
}

NotificationUpdateStatus notification_service_mark_all_read(Database *db,
                                                            const char *scope_user,
                                                            int *updated_out,
                                                            char *errbuf, size_t errlen) {
  if (scope_user == NULL || scope_user[0] == '\0') {
    return NOTIFICATION_UPDATE_BAD_INPUT;
  }
  if (db == NULL || !db->connected) {
    return NOTIFICATION_UPDATE_DB_UNAVAILABLE;
  }
  const char *params[1] = {scope_user};
  DbResult result;
  memset(&result, 0, sizeof(result));
  const char *sql =
      "UPDATE public.notifications SET read = true "
      "WHERE read = false AND (user_id = $1 OR user_id = 'all') RETURNING id";
  if (!db_query(db, sql, params, 1, &result, errbuf, errlen)) {
    db_result_free(&result);
    return NOTIFICATION_UPDATE_QUERY_FAILED;
  }
  if (updated_out != NULL) {
    *updated_out = result.nrows;
  }
  db_result_free(&result);
  return NOTIFICATION_UPDATE_OK;
}
