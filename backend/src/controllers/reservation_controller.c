/**
 * reservation_controller.c - Request handling for reservations.
 *
 * Thin: reads the query through the service, maps ListStatus to HTTP and never
 * leaks internal database errors to the client.
 */
#include "controllers/reservation_controller.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "middleware/auth_middleware.h"
#include "services/reservation_service.h"
#include "utils/json_parse.h"
#include "utils/response_utils.h"

/* One page of reservations (bounded by the shared limit cap). */
#define RESERVATIONS_JSON_BUFFER_SIZE 262144

/* A single reservation is small; generous for the selected columns. */
#define RESERVATION_JSON_BUFFER_SIZE 8192

/* Longest id we accept before answering 404 without touching the database. */
#define RESERVATION_ID_MAX 128

/* Upper bound for a create request body. */
#define RESERVATION_BODY_MAX 8192

static void send_list_status(struct mg_connection *c, ListStatus status, const char *json,
                             const char *errbuf) {
  switch (status) {
    case LIST_OK:
      send_json(c, 200, json);
      break;
    case LIST_DB_UNAVAILABLE:
      send_json_error(c, 503, "database_unavailable",
                      "Reservations are temporarily unavailable.");
      break;
    case LIST_BUFFER_TOO_SMALL:
      fprintf(stderr, "[reservations] response buffer too small for the result set\n");
      send_json_error(c, 500, "response_too_large",
                      "The reservation list is too large to return.");
      break;
    case LIST_NOT_FOUND:
      send_json_error(c, 404, "not_found", "Reservation not found.");
      break;
    case LIST_QUERY_FAILED:
    default:
      fprintf(stderr, "[reservations] query failed: %s\n",
              errbuf[0] ? errbuf : "(no details)");
      send_json_error(c, 500, "database_error", "Could not load reservations right now.");
      break;
  }
}

void reservation_controller_list(AppContext *app, struct mg_connection *c,
                                 const struct mg_http_message *hm) {
  Session *session = auth_require(app, c, hm);
  if (session == NULL) {
    return;
  }
  ReservationListQuery query = {0};
  char sbuf[128];
  char bbuf[128];
  char statbuf[64];
  char numbuf[16];

  if (mg_http_get_var(&hm->query, "student_id", sbuf, sizeof(sbuf)) >= 0) {
    query.student_id = sbuf;
  }
  if (!auth_role_is_admin(session->role)) {
    query.student_id = session->admin_id;
  }
  if (mg_http_get_var(&hm->query, "book_id", bbuf, sizeof(bbuf)) >= 0) {
    query.book_id = bbuf;
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

  char *buffer = malloc(RESERVATIONS_JSON_BUFFER_SIZE);
  if (buffer == NULL) {
    send_json_error(c, 500, "server_error", "The server ran out of memory.");
    return;
  }

  char errbuf[256] = {0};
  ListStatus status = reservation_service_list(
      app ? app->db : NULL, &query, buffer, RESERVATIONS_JSON_BUFFER_SIZE, errbuf,
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

void reservation_controller_get(AppContext *app, struct mg_connection *c,
                                const struct mg_http_message *hm, struct mg_str id) {
  Session *session = auth_require(app, c, hm);
  if (session == NULL) {
    return;
  }

  char idbuf[RESERVATION_ID_MAX];
  if (!copy_segment(idbuf, sizeof(idbuf), id)) {
    send_json_error(c, 404, "not_found", "Reservation not found.");
    return;
  }

  if (!auth_role_is_admin(session->role)) {
    char owner[RESERVATION_ID_MAX];
    char owner_err[256] = {0};
    ReservationOwnerStatus owner_status = reservation_service_get_owner(
        app ? app->db : NULL, idbuf, owner, sizeof(owner), owner_err, sizeof(owner_err));
    if (owner_status != RESERVATION_OWNER_OK) {
      send_json_error(c, owner_status == RESERVATION_OWNER_DB_UNAVAILABLE ? 503 : 404,
                      owner_status == RESERVATION_OWNER_DB_UNAVAILABLE
                          ? "database_unavailable"
                          : "not_found",
                      "Reservation not found.");
      return;
    }
    if (strcmp(owner, session->admin_id) != 0) {
      send_json_error(c, 403, "forbidden", "You may only view your own reservations.");
      return;
    }
  }

  char *buffer = malloc(RESERVATION_JSON_BUFFER_SIZE);
  if (buffer == NULL) {
    send_json_error(c, 500, "server_error", "The server ran out of memory.");
    return;
  }

  char errbuf[256] = {0};
  ListStatus status = reservation_service_get(
      app ? app->db : NULL, idbuf, buffer, RESERVATION_JSON_BUFFER_SIZE, errbuf,
      sizeof(errbuf));
  send_list_status(c, status, buffer, errbuf);
  free(buffer);
}

/* ---- Create / cancel / ready ------------------------------------------- */

/* Load the reservation and reply with {"reservation":{...}} at status_code. */
static void reply_with_reservation_at(AppContext *app, struct mg_connection *c,
                                      int status_code, const char *idbuf) {
  char *buffer = malloc(RESERVATION_JSON_BUFFER_SIZE);
  if (buffer == NULL) {
    send_json_error(c, 500, "server_error", "The server ran out of memory.");
    return;
  }

  char errbuf[256] = {0};
  ListStatus status = reservation_service_get(
      app ? app->db : NULL, idbuf, buffer, RESERVATION_JSON_BUFFER_SIZE, errbuf,
      sizeof(errbuf));
  if (status == LIST_OK) {
    send_json(c, status_code, buffer);
  } else {
    send_list_status(c, status, buffer, errbuf);
  }
  free(buffer);
}

void reservation_controller_create(AppContext *app, struct mg_connection *c,
                                   const struct mg_http_message *hm) {
  Session *session = auth_require(app, c, hm);
  if (session == NULL) {
    return; /* auth_require already answered 401 */
  }

  if (hm->body.len == 0 || hm->body.len >= RESERVATION_BODY_MAX) {
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

  char body[RESERVATION_BODY_MAX];
  memcpy(body, hm->body.buf, hm->body.len);
  body[hm->body.len] = '\0';

  ReservationInput input;
  memset(&input, 0, sizeof(input));
  if (json_get_string(body, "student_id", input.student_id, sizeof(input.student_id)) ==
          JSON_PARSE_ERROR ||
      json_get_string(body, "book_id", input.book_id, sizeof(input.book_id)) ==
          JSON_PARSE_ERROR) {
    send_json_error(c, 400, "invalid_body", "The request body could not be parsed.");
    return;
  }

  if (!auth_role_is_admin(session->role)) {
    snprintf(input.student_id, sizeof(input.student_id), "%s", session->admin_id);
  }

  char idbuf[RESERVATION_ID_MAX];
  char errbuf[256] = {0};
  ReservationCreateStatus status =
      reservation_service_create(app ? app->db : NULL, &input, idbuf, sizeof(idbuf),
                                 errbuf, sizeof(errbuf));

  switch (status) {
    case RESERVATION_CREATE_OK:
      fprintf(stderr, "[reservations] create by '%s' (%s): id=%s book=%s student=%s\n",
              session->admin_id, session->role, idbuf, input.book_id, input.student_id);
      reply_with_reservation_at(app, c, 201, idbuf);
      break;
    case RESERVATION_CREATE_BAD_INPUT:
      send_json_error(c, 400, "invalid_body", "student_id and book_id are required.");
      break;
    case RESERVATION_CREATE_DUPLICATE:
      send_json_error(c, 409, "conflict",
                      "This student already has a waiting reservation for this book.");
      break;
    case RESERVATION_CREATE_NOT_FOUND:
      send_json_error(c, 404, "not_found", "Student or book not found.");
      break;
    case RESERVATION_CREATE_DB_UNAVAILABLE:
      send_json_error(c, 503, "database_unavailable",
                      "Reservations are temporarily unavailable.");
      break;
    case RESERVATION_CREATE_QUERY_FAILED:
    default:
      fprintf(stderr, "[reservations] create failed: %s\n",
              errbuf[0] ? errbuf : "(no details)");
      send_json_error(c, 500, "database_error", "Could not create the reservation right now.");
      break;
  }
}

void reservation_controller_cancel(AppContext *app, struct mg_connection *c,
                                   const struct mg_http_message *hm, struct mg_str id) {
  Session *session = auth_require(app, c, hm);
  if (session == NULL) {
    return; /* auth_require already answered 401 */
  }

  char idbuf[RESERVATION_ID_MAX];
  if (!copy_segment(idbuf, sizeof(idbuf), id)) {
    send_json_error(c, 404, "not_found", "Reservation not found.");
    return;
  }

  if (!auth_role_is_admin(session->role)) {
    char owner[RESERVATION_ID_MAX];
    char owner_err[256] = {0};
    ReservationOwnerStatus owner_status = reservation_service_get_owner(
        app ? app->db : NULL, idbuf, owner, sizeof(owner), owner_err, sizeof(owner_err));
    if (owner_status != RESERVATION_OWNER_OK) {
      send_json_error(c, owner_status == RESERVATION_OWNER_DB_UNAVAILABLE ? 503 : 404,
                      owner_status == RESERVATION_OWNER_DB_UNAVAILABLE
                          ? "database_unavailable"
                          : "not_found",
                      "No active reservation with that id.");
      return;
    }
    if (strcmp(owner, session->admin_id) != 0) {
      send_json_error(c, 403, "forbidden", "You may only cancel your own reservations.");
      return;
    }
  }

  char errbuf[256] = {0};
  ReservationCancelStatus status =
      reservation_service_cancel(app ? app->db : NULL, idbuf, errbuf, sizeof(errbuf));

  switch (status) {
    case RESERVATION_CANCEL_OK:
      fprintf(stderr, "[reservations] cancel by '%s' (%s): id=%s\n", session->admin_id,
              session->role, idbuf);
      send_json(c, 200, "{\"ok\":true}");
      break;
    case RESERVATION_CANCEL_BAD_INPUT:
    case RESERVATION_CANCEL_NOT_FOUND:
      send_json_error(c, 404, "not_found", "No active reservation with that id.");
      break;
    case RESERVATION_CANCEL_DB_UNAVAILABLE:
      send_json_error(c, 503, "database_unavailable",
                      "Reservations are temporarily unavailable.");
      break;
    case RESERVATION_CANCEL_QUERY_FAILED:
    default:
      fprintf(stderr, "[reservations] cancel '%s' failed: %s\n", idbuf,
              errbuf[0] ? errbuf : "(no details)");
      send_json_error(c, 500, "database_error", "Could not cancel the reservation right now.");
      break;
  }
}

void reservation_controller_ready(AppContext *app, struct mg_connection *c,
                                  const struct mg_http_message *hm, struct mg_str id) {
  Session *session = auth_require_admin(app, c, hm);
  if (session == NULL) {
    return; /* auth_require already answered 401 */
  }

  char idbuf[RESERVATION_ID_MAX];
  if (!copy_segment(idbuf, sizeof(idbuf), id)) {
    send_json_error(c, 404, "not_found", "Reservation not found.");
    return;
  }

  char errbuf[256] = {0};
  ReservationReadyStatus status =
      reservation_service_ready(app ? app->db : NULL, idbuf, errbuf, sizeof(errbuf));

  switch (status) {
    case RESERVATION_READY_OK:
      fprintf(stderr, "[reservations] ready by '%s' (%s): id=%s\n", session->admin_id,
              session->role, idbuf);
      reply_with_reservation_at(app, c, 200, idbuf);
      break;
    case RESERVATION_READY_BAD_INPUT:
    case RESERVATION_READY_NOT_FOUND:
      send_json_error(c, 404, "not_found", "No waiting reservation with that id.");
      break;
    case RESERVATION_READY_DB_UNAVAILABLE:
      send_json_error(c, 503, "database_unavailable",
                      "Reservations are temporarily unavailable.");
      break;
    case RESERVATION_READY_QUERY_FAILED:
    default:
      fprintf(stderr, "[reservations] ready '%s' failed: %s\n", idbuf,
              errbuf[0] ? errbuf : "(no details)");
      send_json_error(c, 500, "database_error",
                      "Could not update the reservation right now.");
      break;
  }
}