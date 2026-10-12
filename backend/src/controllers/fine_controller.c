/**
 * fine_controller.c - Request handling for fines.
 *
 * Thin: reads the query through the service, maps ListStatus to HTTP and never
 * leaks internal database errors to the client.
 */
#include "controllers/fine_controller.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "middleware/auth_middleware.h"
#include "services/fine_service.h"
#include "utils/response_utils.h"

/* One page of fines (bounded by the shared limit cap). */
#define FINES_JSON_BUFFER_SIZE 262144

/* A single fine is small; generous for the selected columns. */
#define FINE_JSON_BUFFER_SIZE 8192

/* Longest id we accept before answering 404 without touching the database. */
#define FINE_ID_MAX 128

static void send_list_status(struct mg_connection *c, ListStatus status, const char *json,
                             const char *errbuf) {
  switch (status) {
    case LIST_OK:
      send_json(c, 200, json);
      break;
    case LIST_DB_UNAVAILABLE:
      send_json_error(c, 503, "database_unavailable", "Fines are temporarily unavailable.");
      break;
    case LIST_BUFFER_TOO_SMALL:
      fprintf(stderr, "[fines] response buffer too small for the result set\n");
      send_json_error(c, 500, "response_too_large", "The fine list is too large to return.");
      break;
    case LIST_NOT_FOUND:
      send_json_error(c, 404, "not_found", "Fine not found.");
      break;
    case LIST_QUERY_FAILED:
    default:
      fprintf(stderr, "[fines] query failed: %s\n", errbuf[0] ? errbuf : "(no details)");
      send_json_error(c, 500, "database_error", "Could not load fines right now.");
      break;
  }
}

void fine_controller_list(AppContext *app, struct mg_connection *c,
                          const struct mg_http_message *hm) {
  FineListQuery query = {0};
  char sbuf[128];
  char statbuf[64];
  char numbuf[16];

  if (mg_http_get_var(&hm->query, "student_id", sbuf, sizeof(sbuf)) >= 0) {
    query.student_id = sbuf;
  }
  if (mg_http_get_var(&hm->query, "status", statbuf, sizeof(statbuf)) >= 0) {
    query.status = statbuf;
  }
  if (mg_http_get_var(&hm->query, "limit", numbuf, sizeof(numbuf)) >= 0) {
    query.limit = (int)strtol(numbuf, NULL, 10);
  }
  if (mg_http_get_var(&hm->query, "offset", numbuf, sizeof(numbuf)) >= 0) {
    query.offset = (int)strtol(numbuf, NULL, 10);
  }

  char *buffer = malloc(FINES_JSON_BUFFER_SIZE);
  if (buffer == NULL) {
    send_json_error(c, 500, "server_error", "The server ran out of memory.");
    return;
  }

  char errbuf[256] = {0};
  ListStatus status =
      fine_service_list(app ? app->db : NULL, &query, buffer, FINES_JSON_BUFFER_SIZE, errbuf,
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

void fine_controller_get(AppContext *app, struct mg_connection *c,
                         const struct mg_http_message *hm, struct mg_str id) {
  (void)hm;

  char idbuf[FINE_ID_MAX];
  if (!copy_segment(idbuf, sizeof(idbuf), id)) {
    send_json_error(c, 404, "not_found", "Fine not found.");
    return;
  }

  char *buffer = malloc(FINE_JSON_BUFFER_SIZE);
  if (buffer == NULL) {
    send_json_error(c, 500, "server_error", "The server ran out of memory.");
    return;
  }

  char errbuf[256] = {0};
  ListStatus status = fine_service_get(app ? app->db : NULL, idbuf, buffer,
                                       FINE_JSON_BUFFER_SIZE, errbuf, sizeof(errbuf));
  send_list_status(c, status, buffer, errbuf);
  free(buffer);
}

/* ---- Pay ---------------------------------------------------------------- */

void fine_controller_pay(AppContext *app, struct mg_connection *c,
                         const struct mg_http_message *hm, struct mg_str id) {
  Session *session = auth_require(app, c, hm);
  if (session == NULL) {
    return; /* auth_require already answered 401 */
  }

  char idbuf[FINE_ID_MAX];
  if (!copy_segment(idbuf, sizeof(idbuf), id)) {
    send_json_error(c, 404, "not_found", "Fine not found.");
    return;
  }

  char errbuf[256] = {0};
  FinePayStatus status =
      fine_service_pay(app ? app->db : NULL, idbuf, errbuf, sizeof(errbuf));

  switch (status) {
    case FINE_PAY_OK:
      fprintf(stderr, "[fines] pay by '%s' (%s): id=%s\n", session->admin_id,
              session->role, idbuf);
      send_json(c, 200, "{\"ok\":true}");
      break;
    case FINE_PAY_BAD_INPUT:
    case FINE_PAY_NOT_FOUND:
      send_json_error(c, 404, "not_found", "No pending fine with that id.");
      break;
    case FINE_PAY_DB_UNAVAILABLE:
      send_json_error(c, 503, "database_unavailable", "Fines are temporarily unavailable.");
      break;
    case FINE_PAY_QUERY_FAILED:
    default:
      fprintf(stderr, "[fines] pay '%s' failed: %s\n", idbuf,
              errbuf[0] ? errbuf : "(no details)");
      send_json_error(c, 500, "database_error", "Could not update the fine right now.");
      break;
  }
}