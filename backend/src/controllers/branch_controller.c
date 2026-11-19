/**
 * branch_controller.c - Request handling for library branches.
 */
#include "controllers/branch_controller.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "middleware/auth_middleware.h"
#include "services/branch_service.h"
#include "utils/json_parse.h"
#include "utils/json_utils.h"
#include "utils/response_utils.h"

#define BRANCHES_BUFFER_SIZE 65536
#define BRANCH_BODY_MAX 2048
#define BRANCH_ID_MAX 128

static void send_branch_list_status(struct mg_connection *c, ListStatus status,
                                    const char *json, const char *errbuf) {
  switch (status) {
    case LIST_OK:
      send_json(c, 200, json);
      break;
    case LIST_DB_UNAVAILABLE:
      send_json_error(c, 503, "database_unavailable", "Branches are temporarily unavailable.");
      break;
    case LIST_BUFFER_TOO_SMALL:
      send_json_error(c, 500, "response_too_large", "The branch list is too large.");
      break;
    case LIST_QUERY_FAILED:
    default:
      fprintf(stderr, "[branches] list failed: %s\n", errbuf[0] ? errbuf : "(no details)");
      send_json_error(c, 500, "database_error", "Could not load branches right now.");
      break;
  }
}

static void send_branch_status(struct mg_connection *c, BranchStatus status,
                               const char *errbuf) {
  switch (status) {
    case BRANCH_OK:
      send_json(c, 200, "{\"ok\":true}");
      break;
    case BRANCH_BAD_INPUT:
      send_json_error(c, 400, "invalid_body", "A branch name is required.");
      break;
    case BRANCH_NOT_FOUND:
      send_json_error(c, 404, "not_found", "Branch not found.");
      break;
    case BRANCH_CONFLICT:
      send_json_error(c, 409, "conflict", "That branch already exists or is in use.");
      break;
    case BRANCH_DB_UNAVAILABLE:
      send_json_error(c, 503, "database_unavailable", "Branches are temporarily unavailable.");
      break;
    case BRANCH_QUERY_FAILED:
    default:
      fprintf(stderr, "[branches] write failed: %s\n", errbuf[0] ? errbuf : "(no details)");
      send_json_error(c, 500, "database_error", "Could not update branches right now.");
      break;
  }
}

void branch_controller_list(AppContext *app, struct mg_connection *c,
                            const struct mg_http_message *hm) {
  Session *session = auth_require(app, c, hm);
  if (session == NULL) {
    return;
  }
  (void)session;

  char *buffer = malloc(BRANCHES_BUFFER_SIZE);
  if (buffer == NULL) {
    send_json_error(c, 500, "server_error", "The server ran out of memory.");
    return;
  }
  char errbuf[256] = {0};
  ListStatus status = branch_service_list(app ? app->db : NULL, buffer,
                                          BRANCHES_BUFFER_SIZE, errbuf, sizeof(errbuf));
  send_branch_list_status(c, status, buffer, errbuf);
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

static bool read_name(struct mg_connection *c, const struct mg_http_message *hm, char *name,
                      size_t name_size, char *id, size_t id_size) {
  if (hm->body.len == 0 || hm->body.len >= BRANCH_BODY_MAX) {
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
  char body[BRANCH_BODY_MAX];
  memcpy(body, hm->body.buf, hm->body.len);
  body[hm->body.len] = '\0';
  if (json_get_string(body, "name", name, name_size) == JSON_PARSE_ERROR) {
    send_json_error(c, 400, "invalid_body", "The request body could not be parsed.");
    return false;
  }
  if (id != NULL && id_size > 0) {
    if (json_get_string(body, "id", id, id_size) == JSON_PARSE_ERROR) {
      send_json_error(c, 400, "invalid_body", "The request body could not be parsed.");
      return false;
    }
  }
  if (name[0] == '\0') {
    send_json_error(c, 400, "invalid_body", "A branch name is required.");
    return false;
  }
  return true;
}

void branch_controller_create(AppContext *app, struct mg_connection *c,
                              const struct mg_http_message *hm) {
  Session *session = auth_require_admin(app, c, hm);
  if (session == NULL) {
    return;
  }
  (void)session;

  char name[256] = {0};
  char id[BRANCH_ID_MAX] = {0};
  if (!read_name(c, hm, name, sizeof(name), id, sizeof(id))) {
    return;
  }

  char id_out[BRANCH_ID_MAX];
  char errbuf[256] = {0};
  BranchStatus status = branch_service_create(app ? app->db : NULL, id, name, id_out,
                                              sizeof(id_out), errbuf, sizeof(errbuf));
  if (status != BRANCH_OK) {
    send_branch_status(c, status, errbuf);
    return;
  }

  char response[768];
  char esc_id[513], esc_name[513];
  json_escape(id_out, esc_id, sizeof(esc_id));
  json_escape(name, esc_name, sizeof(esc_name));
  snprintf(response, sizeof(response), "{\"branch\":{\"id\":\"%s\",\"name\":\"%s\"}}",
           esc_id, esc_name);
  send_json(c, 201, response);
}

void branch_controller_update(AppContext *app, struct mg_connection *c,
                              const struct mg_http_message *hm, struct mg_str id_seg) {
  Session *session = auth_require_admin(app, c, hm);
  if (session == NULL) {
    return;
  }
  (void)session;

  char id[BRANCH_ID_MAX];
  if (!copy_segment(id, sizeof(id), id_seg)) {
    send_json_error(c, 404, "not_found", "Branch not found.");
    return;
  }
  char name[256] = {0};
  if (!read_name(c, hm, name, sizeof(name), NULL, 0)) {
    return;
  }
  char errbuf[256] = {0};
  BranchStatus status = branch_service_update(app ? app->db : NULL, id, name, errbuf,
                                              sizeof(errbuf));
  send_branch_status(c, status, errbuf);
}

void branch_controller_delete(AppContext *app, struct mg_connection *c,
                              const struct mg_http_message *hm, struct mg_str id_seg) {
  Session *session = auth_require_admin(app, c, hm);
  if (session == NULL) {
    return;
  }
  (void)session;

  char id[BRANCH_ID_MAX];
  if (!copy_segment(id, sizeof(id), id_seg)) {
    send_json_error(c, 404, "not_found", "Branch not found.");
    return;
  }
  char errbuf[256] = {0};
  BranchStatus status = branch_service_delete(app ? app->db : NULL, id, errbuf,
                                              sizeof(errbuf));
  send_branch_status(c, status, errbuf);
}
