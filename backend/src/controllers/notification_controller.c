/**
 * notification_controller.c - Request handling for notifications.
 *
 * Thin: reads the query through the service, maps ListStatus to HTTP and never
 * leaks internal database errors to the client.
 */
#include "controllers/notification_controller.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "services/notification_service.h"
#include "utils/response_utils.h"

/* One page of notifications (bounded by the shared limit cap). */
#define NOTIFICATIONS_JSON_BUFFER_SIZE 131072

/* A single notification is small; generous for the selected columns. */
#define NOTIFICATION_JSON_BUFFER_SIZE 8192

/* Longest id we accept before answering 404 without touching the database. */
#define NOTIFICATION_ID_MAX 128

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
  NotificationListQuery query = {0};
  char ubuf[128];
  char tbuf[64];
  char numbuf[16];

  if (mg_http_get_var(&hm->query, "user_id", ubuf, sizeof(ubuf)) >= 0) {
    query.user_id = ubuf;
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

/* Copy a URL slice into a NUL-terminated buffer; returns false when too long. */
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
  (void)hm;

  char idbuf[NOTIFICATION_ID_MAX];
  if (!copy_segment(idbuf, sizeof(idbuf), id)) {
    send_json_error(c, 404, "not_found", "Notification not found.");
    return;
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