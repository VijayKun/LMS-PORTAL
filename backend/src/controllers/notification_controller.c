/**
 * notification_controller.c - Request handling for notifications.
 *
 * Reads are scoped to the signed-in user; announcements and global read-state
 * changes require an administrator.
 */
#include "controllers/notification_controller.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "middleware/auth_middleware.h"
#include "services/notification_service.h"
#include "utils/json_parse.h"
#include "utils/response_utils.h"

#define NOTIFICATIONS_JSON_BUFFER_SIZE 131072
#define NOTIFICATION_JSON_BUFFER_SIZE 8192
#define NOTIFICATION_ID_MAX 128
#define NOTIFICATION_BODY_MAX 8192

static void send_list_status(struct mg_connection *c, ListStatus status, const char *json,
                             const char *errbuf) {
  switch (status) {
    case LIST_OK:
      send_json(c, 200, json);
      break;
    case LIST_DB_UNAVAILABLE:
      send_json_error(c, 503, "database_unavailable",
                      "Notifications are temporarily unavailable.");
      break;
    case LIST_BUFFER_TOO_SMALL:
      fprintf(stderr, "[notifications] response buffer too small for the result set\n");
      send_json_error(c, 500, "response_too_large",
                      "The notification list is too large to return.");
      break;
    case LIST_NOT_FOUND:
      send_json_error(c, 404, "not_found", "Notification not found.");
      break;
    case LIST_QUERY_FAILED:
    default:
      fprintf(stderr, "[notifications] query failed: %s\n",
              errbuf[0] ? errbuf : "(no details)");
      send_json_error(c, 500, "database_error", "Could not load notifications right now.");
      break;
  }
}

void notification_controller_list(AppContext *app, struct mg_connection *c,
                                  const struct mg_http_message *hm) {
  Session *session = auth_require(app, c, hm);
  if (session == NULL) {
    return;
  }

  NotificationListQuery query = {0};
  char ubuf[128];
  char tbuf[64];
  char numbuf[16];

  if (mg_http_get_var(&hm->query, "user_id", ubuf, sizeof(ubuf)) >= 0) {
    query.user_id = ubuf;
  }
  if (!auth_role_is_admin(session->role)) {
    query.user_id = session->admin_id;
  }
  if (mg_http_get_var(&hm->query, "type", tbuf, sizeof(tbuf)) >= 0) {
    query.type = tbuf;
  }
  if (mg_http_get_var(&hm->query, "limit", numbuf, sizeof(numbuf)) >= 0) {
    query.limit = (int)strtol(numbuf, NULL, 10);
  }
  if (mg_http_get_var(&hm->query, "offset", numbuf, sizeof(numbuf)) >= 0) {
    query.offset = (int)strtol(numbuf, NULL, 10);
  }

  char *buffer = malloc(NOTIFICATIONS_JSON_BUFFER_SIZE);
  if (buffer == NULL) {
    send_json_error(c, 500, "server_error", "The server ran out of memory.");
    return;
  }

  char errbuf[256] = {0};
  ListStatus status = notification_service_list(
      app ? app->db : NULL, &query, buffer, NOTIFICATIONS_JSON_BUFFER_SIZE, errbuf,
      sizeof(errbuf));
  send_list_status(c, status, buffer, errbuf);
  free(buffer);
}

static bool copy_segment(char *dst, size_t dst_size, struct mg_str seg) {
  if (seg.len == 0 || seg.len >= dst_size) {
    return false;
  }
  memcpy(dst, seg.buf, seg.len);
  dst[seg.len] = '\0';
  return true;
}

void notification_controller_get(AppContext *app, struct mg_connection *c,
                                 const struct mg_http_message *hm, struct mg_str id) {
  Session *session = auth_require(app, c, hm);
  if (session == NULL) {
    return;
  }

  char idbuf[NOTIFICATION_ID_MAX];
  if (!copy_segment(idbuf, sizeof(idbuf), id)) {
    send_json_error(c, 404, "not_found", "Notification not found.");
    return;
  }

  if (!auth_role_is_admin(session->role)) {
    char owner[NOTIFICATION_ID_MAX];
    char owner_err[256] = {0};
    NotificationOwnerStatus owner_status = notification_service_get_owner(
        app ? app->db : NULL, idbuf, owner, sizeof(owner), owner_err, sizeof(owner_err));
    if (owner_status != NOTIFICATION_OWNER_OK) {
      send_json_error(c, owner_status == NOTIFICATION_OWNER_DB_UNAVAILABLE ? 503 : 404,
                      owner_status == NOTIFICATION_OWNER_DB_UNAVAILABLE
                          ? "database_unavailable"
                          : "not_found",
                      "Notification not found.");
      return;
    }
    if (strcmp(owner, session->admin_id) != 0 && strcmp(owner, "all") != 0) {
      send_json_error(c, 403, "forbidden", "You may only view your own notifications.");
      return;
    }
  }

  char *buffer = malloc(NOTIFICATION_JSON_BUFFER_SIZE);
  if (buffer == NULL) {
    send_json_error(c, 500, "server_error", "The server ran out of memory.");
    return;
  }

  char errbuf[256] = {0};
  ListStatus status = notification_service_get(
      app ? app->db : NULL, idbuf, buffer, NOTIFICATION_JSON_BUFFER_SIZE, errbuf,
      sizeof(errbuf));
  send_list_status(c, status, buffer, errbuf);
  free(buffer);
}

static void send_update_status(struct mg_connection *c, NotificationUpdateStatus status,
                               int updated, const char *errbuf) {
  switch (status) {
    case NOTIFICATION_UPDATE_OK:
      send_json(c, 200, updated >= 0 ? "{\"ok\":true}" : "{\"ok\":true}");
      break;
    case NOTIFICATION_UPDATE_NOT_FOUND:
      send_json_error(c, 404, "not_found", "Notification not found.");
      break;
    case NOTIFICATION_UPDATE_BAD_INPUT:
      send_json_error(c, 400, "invalid_body", "A notification id is required.");
      break;
    case NOTIFICATION_UPDATE_DB_UNAVAILABLE:
      send_json_error(c, 503, "database_unavailable",
                      "Notifications are temporarily unavailable.");
      break;
    case NOTIFICATION_UPDATE_QUERY_FAILED:
    default:
      fprintf(stderr, "[notifications] update failed: %s\n",
              errbuf[0] ? errbuf : "(no details)");
      send_json_error(c, 500, "database_error", "Could not update notifications right now.");
      break;
  }
}

void notification_controller_create(AppContext *app, struct mg_connection *c,
                                    const struct mg_http_message *hm) {
  Session *session = auth_require_admin(app, c, hm);
  if (session == NULL) {
    return;
  }
  (void)session;

  if (hm->body.len == 0 || hm->body.len >= NOTIFICATION_BODY_MAX) {
    send_json_error(c, 400, "invalid_body", "A JSON body is required.");
    return;
  }
  struct mg_str *content_type = mg_http_get_header(hm, "Content-Type");
  if (content_type == NULL ||
      mg_strcasecmp(*content_type, mg_str("application/json")) != 0) {
    send_json_error(c, 415, "unsupported_media_type",
                    "Content-Type must be application/json.");
    return;
  }

  char body[NOTIFICATION_BODY_MAX];
  memcpy(body, hm->body.buf, hm->body.len);
  body[hm->body.len] = '\0';

  NotificationInput input;
  memset(&input, 0, sizeof(input));
  if (json_get_string(body, "type", input.type, sizeof(input.type)) == JSON_PARSE_ERROR ||
      json_get_string(body, "title", input.title, sizeof(input.title)) ==
          JSON_PARSE_ERROR ||
      json_get_string(body, "message", input.message, sizeof(input.message)) ==
          JSON_PARSE_ERROR ||
      json_get_string(body, "user_id", input.user_id, sizeof(input.user_id)) ==
          JSON_PARSE_ERROR) {
    send_json_error(c, 400, "invalid_body", "The request body could not be parsed.");
    return;
  }
  if (input.type[0] == '\0') {
    snprintf(input.type, sizeof(input.type), "announcement");
  }

  char idbuf[NOTIFICATION_ID_MAX];
  char errbuf[256] = {0};
  NotificationCreateStatus status = notification_service_create(
      app ? app->db : NULL, &input, idbuf, sizeof(idbuf), errbuf, sizeof(errbuf));
  switch (status) {
    case NOTIFICATION_CREATE_OK:
      fprintf(stderr, "[notifications] announcement created: id=%s user=%s\n", idbuf,
              input.user_id[0] ? input.user_id : "all");
      send_json(c, 201, "{\"ok\":true}");
      break;
    case NOTIFICATION_CREATE_BAD_INPUT:
      send_json_error(c, 400, "invalid_body", "type and title are required.");
      break;
    case NOTIFICATION_CREATE_DB_UNAVAILABLE:
      send_json_error(c, 503, "database_unavailable",
                      "Notifications are temporarily unavailable.");
      break;
    case NOTIFICATION_CREATE_QUERY_FAILED:
    default:
      fprintf(stderr, "[notifications] create failed: %s\n",
              errbuf[0] ? errbuf : "(no details)");
      send_json_error(c, 500, "database_error", "Could not create the announcement.");
      break;
  }
}

void notification_controller_mark_read(AppContext *app, struct mg_connection *c,
                                       const struct mg_http_message *hm, struct mg_str id) {
  Session *session = auth_require(app, c, hm);
  if (session == NULL) {
    return;
  }
  char idbuf[NOTIFICATION_ID_MAX];
  if (!copy_segment(idbuf, sizeof(idbuf), id)) {
    send_json_error(c, 404, "not_found", "Notification not found.");
    return;
  }

  char errbuf[256] = {0};
  NotificationUpdateStatus status = notification_service_mark_read(
      app ? app->db : NULL, idbuf,
      auth_role_is_admin(session->role) ? NULL : session->admin_id, errbuf,
      sizeof(errbuf));
  send_update_status(c, status, 0, errbuf);
}

void notification_controller_mark_all_read(AppContext *app, struct mg_connection *c,
                                           const struct mg_http_message *hm) {
  Session *session = auth_require(app, c, hm);
  if (session == NULL) {
    return;
  }
  int updated = 0;
  char errbuf[256] = {0};
  NotificationUpdateStatus status = notification_service_mark_all_read(
      app ? app->db : NULL, session->admin_id, &updated, errbuf, sizeof(errbuf));
  if (status == NOTIFICATION_UPDATE_OK) {
    char response[64];
    snprintf(response, sizeof(response), "{\"ok\":true,\"updated\":%d}", updated);
    send_json(c, 200, response);
    return;
  }
  send_update_status(c, status, updated, errbuf);
}
