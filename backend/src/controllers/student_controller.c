/**
 * student_controller.c - Request handling for the student directory.
 *
 * Reads through the service, maps service status to an HTTP response and never
 * leaks connection strings or internal database errors to the client.
 */
#include "controllers/student_controller.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "middleware/auth_middleware.h"
#include "services/student_service.h"
#include "utils/json_parse.h"
#include "utils/response_utils.h"

/* Plenty for one page of the directory (bounded by the limit cap). */
#define STUDENTS_JSON_BUFFER_SIZE 65536

/* A single student is small; generous for the selected columns. */
#define STUDENT_JSON_BUFFER_SIZE 8192

/* Longest id we accept before answering 404 without touching the database. */
#define STUDENT_ID_MAX 128

/* Upper bound for a create/update request body. */
#define STUDENT_BODY_MAX 8192

void student_controller_list(AppContext *app, struct mg_connection *c,
                             const struct mg_http_message *hm) {
  Session *session = auth_require_admin(app, c, hm);
  if (session == NULL) {
    return;
  }
  (void)session;

  StudentListQuery query = {0};
  char qbuf[128];
  char branchbuf[128];
  char numbuf[16];

  if (mg_http_get_var(&hm->query, "q", qbuf, sizeof(qbuf)) >= 0) {
    query.q = qbuf;
  }
  if (mg_http_get_var(&hm->query, "branch", branchbuf, sizeof(branchbuf)) >= 0) {
    query.branch = branchbuf;
  }
  if (mg_http_get_var(&hm->query, "limit", numbuf, sizeof(numbuf)) >= 0) {
    query.limit = (int)strtol(numbuf, NULL, 10);
  }
  if (mg_http_get_var(&hm->query, "offset", numbuf, sizeof(numbuf)) >= 0) {
    query.offset = (int)strtol(numbuf, NULL, 10);
  }

  char *buffer = malloc(STUDENTS_JSON_BUFFER_SIZE);
  if (buffer == NULL) {
    send_json_error(c, 500, "server_error", "The server ran out of memory.");
    return;
  }

  char errbuf[256] = {0};
  StudentListStatus status =
      student_service_search(app ? app->db : NULL, &query, buffer, STUDENTS_JSON_BUFFER_SIZE,
                             errbuf, sizeof(errbuf));

  switch (status) {
    case STUDENT_LIST_OK:
      send_json(c, 200, buffer);
      break;

    case STUDENT_LIST_DB_UNAVAILABLE:
      send_json_error(c, 503, "database_unavailable",
                      "The student directory is temporarily unavailable.");
      break;

    case STUDENT_LIST_BUFFER_TOO_SMALL:
      fprintf(stderr, "[students] response buffer too small for the result set\n");
      send_json_error(c, 500, "response_too_large",
                      "The student list is too large to return.");
      break;

    case STUDENT_LIST_QUERY_FAILED:
    default:
      /* Log server-side only; the client gets a generic message. */
      fprintf(stderr, "[students] query failed: %s\n", errbuf[0] ? errbuf : "(no details)");
      send_json_error(c, 500, "database_error", "Could not load students right now.");
      break;
  }

  free(buffer);
}

void student_controller_get(AppContext *app, struct mg_connection *c,
                            const struct mg_http_message *hm, struct mg_str id) {
  Session *session = auth_require(app, c, hm);
  if (session == NULL) {
    return;
  }

  if (id.len == 0 || id.len >= STUDENT_ID_MAX) {
    send_json_error(c, 404, "not_found", "Student not found.");
    return;
  }
  char idbuf[STUDENT_ID_MAX];
  memcpy(idbuf, id.buf, id.len);
  idbuf[id.len] = '\0';
  if (!auth_role_is_admin(session->role) && strcmp(idbuf, session->admin_id) != 0) {
    send_json_error(c, 403, "forbidden", "You may only view your own profile.");
    return;
  }

  char *buffer = malloc(STUDENT_JSON_BUFFER_SIZE);
  if (buffer == NULL) {
    send_json_error(c, 500, "server_error", "The server ran out of memory.");
    return;
  }

  char errbuf[256] = {0};
  StudentGetStatus status =
      student_service_get(app ? app->db : NULL, idbuf, buffer, STUDENT_JSON_BUFFER_SIZE,
                          errbuf, sizeof(errbuf));

  switch (status) {
    case STUDENT_GET_OK:
      send_json(c, 200, buffer);
      break;

    case STUDENT_GET_NOT_FOUND:
      send_json_error(c, 404, "not_found", "Student not found.");
      break;

    case STUDENT_GET_DB_UNAVAILABLE:
      send_json_error(c, 503, "database_unavailable",
                      "The student directory is temporarily unavailable.");
      break;

    case STUDENT_GET_BUFFER_TOO_SMALL:
      fprintf(stderr, "[students] response buffer too small for student '%s'\n", idbuf);
      send_json_error(c, 500, "response_too_large", "The student could not be returned.");
      break;

    case STUDENT_GET_QUERY_FAILED:
    default:
      fprintf(stderr, "[students] get '%s' failed: %s\n", idbuf,
              errbuf[0] ? errbuf : "(no details)");
      send_json_error(c, 500, "database_error", "Could not load the student right now.");
      break;
  }

  free(buffer);
}

/* ---- Create / update / delete ------------------------------------------ */

/*
 * Read the JSON body into a zeroed StudentMutate. A field that is absent stays
 * empty ("not provided"); present fields copy into fixed-size buffers, so
 * oversized values fail here with 400.
 */
static bool parse_student_body(AppContext *app, struct mg_connection *c,
                               const struct mg_http_message *hm, StudentMutate *out) {
  if (hm->body.len == 0 || hm->body.len >= STUDENT_BODY_MAX) {
    send_json_error(c, 400, "invalid_body", "A JSON body is required.");
    return false;
  }

  struct mg_str *content_type = mg_http_get_header(hm, "Content-Type");
  if (content_type == NULL ||
      mg_strcasecmp(*content_type, mg_str("application/json")) != 0) {
    send_json_error(c, 415, "unsupported_media_type",
                    "Content-Type must be application/json.");
    return false;
  }

  char body[STUDENT_BODY_MAX];
  memcpy(body, hm->body.buf, hm->body.len);
  body[hm->body.len] = '\0';

  memset(out, 0, sizeof(*out));

  struct {
    const char *key;
    char *dst;
    size_t size;
  } fields[] = {
      {"name", out->name, sizeof(out->name)},
      {"email", out->email, sizeof(out->email)},
      {"student_id", out->student_id, sizeof(out->student_id)},
      {"branch", out->branch, sizeof(out->branch)},
      {"program", out->program, sizeof(out->program)},
      {"semester", out->semester, sizeof(out->semester)},
      {"contact", out->contact, sizeof(out->contact)},
      {"joined_date", out->joined_date, sizeof(out->joined_date)},
      {"status", out->status, sizeof(out->status)},
  };

  for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); i++) {
    JsonParseResult r = json_get_string(body, fields[i].key, fields[i].dst, fields[i].size);
    if (r == JSON_PARSE_ERROR) {
      send_json_error(c, 400, "invalid_body", "The request body could not be parsed.");
      return false;
    }
  }

  (void)app;
  return true;
}

/* Load the created/updated row and reply 2xx with the fresh student object. */
static void reply_with_student(AppContext *app, struct mg_connection *c, int status_code,
                               const char *idbuf) {
  char *buffer = malloc(STUDENT_JSON_BUFFER_SIZE);
  if (buffer == NULL) {
    send_json_error(c, 500, "server_error", "The server ran out of memory.");
    return;
  }

  char errbuf[256] = {0};
  StudentGetStatus gs = student_service_get(app ? app->db : NULL, idbuf, buffer,
                                            STUDENT_JSON_BUFFER_SIZE, errbuf, sizeof(errbuf));
  if (gs == STUDENT_GET_OK) {
    send_json(c, status_code, buffer);
  } else {
    send_json_error(c, 500, "database_error", "Could not load the student right now.");
  }
  free(buffer);
}

void student_controller_create(AppContext *app, struct mg_connection *c,
                               const struct mg_http_message *hm) {
  Session *session = auth_require_admin(app, c, hm);
  if (session == NULL) {
    return; /* auth_require already answered 401 */
  }

  StudentMutate student;
  if (!parse_student_body(app, c, hm, &student)) {
    return;
  }

  char idbuf[STUDENT_ID_MAX];
  char errbuf[256] = {0};
  StudentCreateStatus status = student_service_create(app ? app->db : NULL, &student, idbuf,
                                                      sizeof(idbuf), errbuf, sizeof(errbuf));

  switch (status) {
    case STUDENT_CREATE_OK:
      fprintf(stderr, "[students] create by '%s' (%s): id=%s\n", session->admin_id,
              session->role, idbuf);
      reply_with_student(app, c, 201, idbuf);
      break;
    case STUDENT_CREATE_BAD_INPUT:
      send_json_error(c, 400, "invalid_body",
                      "Name, email and student_id are required; values must be valid.");
      break;
    case STUDENT_CREATE_DUPLICATE:
      send_json_error(c, 409, "conflict",
                      "A student with this email or student_id already exists.");
      break;
    case STUDENT_CREATE_DB_UNAVAILABLE:
      send_json_error(c, 503, "database_unavailable",
                      "The student directory is temporarily unavailable.");
      break;
    case STUDENT_CREATE_QUERY_FAILED:
    default:
      fprintf(stderr, "[students] create failed: %s\n", errbuf[0] ? errbuf : "(no details)");
      send_json_error(c, 500, "database_error", "Could not create the student right now.");
      break;
  }
}

void student_controller_update(AppContext *app, struct mg_connection *c,
                               const struct mg_http_message *hm, struct mg_str id) {
  Session *session = auth_require_admin(app, c, hm);
  if (session == NULL) {
    return; /* auth_require already answered 401 */
  }

  if (id.len == 0 || id.len >= STUDENT_ID_MAX) {
    send_json_error(c, 404, "not_found", "Student not found.");
    return;
  }
  char idbuf[STUDENT_ID_MAX];
  memcpy(idbuf, id.buf, id.len);
  idbuf[id.len] = '\0';

  StudentMutate student;
  if (!parse_student_body(app, c, hm, &student)) {
    return;
  }

  char errbuf[256] = {0};
  StudentUpdateStatus status =
      student_service_update(app ? app->db : NULL, idbuf, &student, errbuf, sizeof(errbuf));

  switch (status) {
    case STUDENT_UPDATE_OK:
      fprintf(stderr, "[students] update by '%s' (%s): id=%s\n", session->admin_id,
              session->role, idbuf);
      reply_with_student(app, c, 200, idbuf);
      break;
    case STUDENT_UPDATE_NOT_FOUND:
      send_json_error(c, 404, "not_found", "Student not found.");
      break;
    case STUDENT_UPDATE_BAD_INPUT:
      send_json_error(c, 400, "invalid_body",
                      "At least one editable field is required and values must be valid.");
      break;
    case STUDENT_UPDATE_DUPLICATE:
      send_json_error(c, 409, "conflict",
                      "A student with this email or student_id already exists.");
      break;
    case STUDENT_UPDATE_DB_UNAVAILABLE:
      send_json_error(c, 503, "database_unavailable",
                      "The student directory is temporarily unavailable.");
      break;
    case STUDENT_UPDATE_QUERY_FAILED:
    default:
      fprintf(stderr, "[students] update '%s' failed: %s\n", idbuf,
              errbuf[0] ? errbuf : "(no details)");
      send_json_error(c, 500, "database_error", "Could not update the student right now.");
      break;
  }
}

void student_controller_delete(AppContext *app, struct mg_connection *c,
                               const struct mg_http_message *hm, struct mg_str id) {
  Session *session = auth_require_admin(app, c, hm);
  if (session == NULL) {
    return; /* auth_require already answered 401 */
  }

  if (id.len == 0 || id.len >= STUDENT_ID_MAX) {
    send_json_error(c, 404, "not_found", "Student not found.");
    return;
  }
  char idbuf[STUDENT_ID_MAX];
  memcpy(idbuf, id.buf, id.len);
  idbuf[id.len] = '\0';

  char errbuf[256] = {0};
  StudentDeleteStatus status =
      student_service_delete(app ? app->db : NULL, idbuf, errbuf, sizeof(errbuf));

  switch (status) {
    case STUDENT_DELETE_OK:
      fprintf(stderr, "[students] delete by '%s' (%s): id=%s\n", session->admin_id,
              session->role, idbuf);
      send_json(c, 200, "{\"ok\":true}");
      break;
    case STUDENT_DELETE_NOT_FOUND:
      send_json_error(c, 404, "not_found", "Student not found.");
      break;
    case STUDENT_DELETE_DB_UNAVAILABLE:
      send_json_error(c, 503, "database_unavailable",
                      "The student directory is temporarily unavailable.");
      break;
    case STUDENT_DELETE_QUERY_FAILED:
    default:
      fprintf(stderr, "[students] delete '%s' failed: %s\n", idbuf,
              errbuf[0] ? errbuf : "(no details)");
      send_json_error(c, 500, "database_error", "Could not delete the student right now.");
      break;
  }
}
