/**
 * loan_controller.c - Request handling for loans.
 *
 * Thin: reads the query through the service, maps ListStatus to HTTP and never
 * leaks internal database errors to the client.
 */
#include "controllers/loan_controller.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "middleware/auth_middleware.h"
#include "services/loan_service.h"
#include "utils/json_parse.h"
#include "utils/response_utils.h"

/* One page of loans (bounded by the shared limit cap). */
#define LOANS_JSON_BUFFER_SIZE 262144

/* A single loan is small; generous for the selected columns. */
#define LOAN_JSON_BUFFER_SIZE 8192

/* Longest id we accept before answering 404 without touching the database. */
#define LOAN_ID_MAX 128

/* Upper bound for an issue request body. */
#define LOAN_BODY_MAX 8192

static void send_list_status(struct mg_connection *c, ListStatus status, const char *json,
                             const char *errbuf) {
  switch (status) {
    case LIST_OK:
      send_json(c, 200, json);
      break;
    case LIST_DB_UNAVAILABLE:
      send_json_error(c, 503, "database_unavailable",
                      "Loans are temporarily unavailable.");
      break;
    case LIST_BUFFER_TOO_SMALL:
      fprintf(stderr, "[loans] response buffer too small for the result set\n");
      send_json_error(c, 500, "response_too_large", "The loan list is too large to return.");
      break;
    case LIST_NOT_FOUND:
      send_json_error(c, 404, "not_found", "Loan not found.");
      break;
    case LIST_QUERY_FAILED:
    default:
      fprintf(stderr, "[loans] query failed: %s\n", errbuf[0] ? errbuf : "(no details)");
      send_json_error(c, 500, "database_error", "Could not load loans right now.");
      break;
  }
}

void loan_controller_list(AppContext *app, struct mg_connection *c,
                          const struct mg_http_message *hm) {
  LoanListQuery query = {0};
  char sbuf[128];
  char bbuf[128];
  char statbuf[64];
  char numbuf[16];

  if (mg_http_get_var(&hm->query, "student_id", sbuf, sizeof(sbuf)) >= 0) {
    query.student_id = sbuf;
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

  char *buffer = malloc(LOANS_JSON_BUFFER_SIZE);
  if (buffer == NULL) {
    send_json_error(c, 500, "server_error", "The server ran out of memory.");
    return;
  }

  char errbuf[256] = {0};
  ListStatus status =
      loan_service_list(app ? app->db : NULL, &query, buffer, LOANS_JSON_BUFFER_SIZE,
                        errbuf, sizeof(errbuf));
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

void loan_controller_get(AppContext *app, struct mg_connection *c,
                         const struct mg_http_message *hm, struct mg_str id) {
  (void)hm;

  char idbuf[LOAN_ID_MAX];
  if (!copy_segment(idbuf, sizeof(idbuf), id)) {
    send_json_error(c, 404, "not_found", "Loan not found.");
    return;
  }

  char *buffer = malloc(LOAN_JSON_BUFFER_SIZE);
  if (buffer == NULL) {
    send_json_error(c, 500, "server_error", "The server ran out of memory.");
    return;
  }

  char errbuf[256] = {0};
  ListStatus status = loan_service_get(app ? app->db : NULL, idbuf, buffer,
                                       LOAN_JSON_BUFFER_SIZE, errbuf, sizeof(errbuf));
  send_list_status(c, status, buffer, errbuf);
  free(buffer);
}

/* ---- Issue / return ----------------------------------------------------- */

/* Load the affected loan and reply with {"loan":{...}} at the given status. */
static void reply_with_loan_at(AppContext *app, struct mg_connection *c, int status_code,
                               const char *idbuf) {
  char *buffer = malloc(LOAN_JSON_BUFFER_SIZE);
  if (buffer == NULL) {
    send_json_error(c, 500, "server_error", "The server ran out of memory.");
    return;
  }

  char errbuf[256] = {0};
  ListStatus status = loan_service_get(app ? app->db : NULL, idbuf, buffer,
                                       LOAN_JSON_BUFFER_SIZE, errbuf, sizeof(errbuf));
  if (status == LIST_OK) {
    send_json(c, status_code, buffer);
  } else {
    send_list_status(c, status, buffer, errbuf);
  }
  free(buffer);
}

void loan_controller_issue(AppContext *app, struct mg_connection *c,
                           const struct mg_http_message *hm) {
  Session *session = auth_require(app, c, hm);
  if (session == NULL) {
    return; /* auth_require already answered 401 */
  }

  if (hm->body.len == 0 || hm->body.len >= LOAN_BODY_MAX) {
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

  char body[LOAN_BODY_MAX];
  memcpy(body, hm->body.buf, hm->body.len);
  body[hm->body.len] = '\0';

  LoanIssueInput input;
  memset(&input, 0, sizeof(input));
  if (json_get_string(body, "student_id", input.student_id, sizeof(input.student_id)) ==
          JSON_PARSE_ERROR ||
      json_get_string(body, "book_id", input.book_id, sizeof(input.book_id)) ==
          JSON_PARSE_ERROR ||
      json_get_string(body, "due_date", input.due_date, sizeof(input.due_date)) ==
          JSON_PARSE_ERROR) {
    send_json_error(c, 400, "invalid_body", "The request body could not be parsed.");
    return;
  }

  char idbuf[LOAN_ID_MAX];
  char errbuf[256] = {0};
  LoanIssueStatus status = loan_service_issue(app ? app->db : NULL, &input, idbuf,
                                              sizeof(idbuf), errbuf, sizeof(errbuf));

  switch (status) {
    case LOAN_ISSUE_OK:
      fprintf(stderr, "[loans] issue by '%s' (%s): id=%s book=%s student=%s\n",
              session->admin_id, session->role, idbuf, input.book_id, input.student_id);
      reply_with_loan_at(app, c, 201, idbuf);
      break;
    case LOAN_ISSUE_BAD_INPUT:
      send_json_error(c, 400, "invalid_body",
                      "student_id and book_id are required; due_date must be YYYY-MM-DD.");
      break;
    case LOAN_ISSUE_BOOK_NOT_FOUND:
      send_json_error(c, 404, "not_found", "Book not found.");
      break;
    case LOAN_ISSUE_STUDENT_NOT_FOUND:
      send_json_error(c, 404, "not_found", "Student not found.");
      break;
    case LOAN_ISSUE_NO_COPY_AVAILABLE:
      send_json_error(c, 409, "conflict", "No copy of this book is available.");
      break;
    case LOAN_ISSUE_DB_UNAVAILABLE:
      send_json_error(c, 503, "database_unavailable", "Loans are temporarily unavailable.");
      break;
    case LOAN_ISSUE_QUERY_FAILED:
    default:
      fprintf(stderr, "[loans] issue failed: %s\n", errbuf[0] ? errbuf : "(no details)");
      send_json_error(c, 500, "database_error", "Could not issue the book right now.");
      break;
  }
}

void loan_controller_return(AppContext *app, struct mg_connection *c,
                            const struct mg_http_message *hm, struct mg_str id) {
  Session *session = auth_require(app, c, hm);
  if (session == NULL) {
    return; /* auth_require already answered 401 */
  }

  char idbuf[LOAN_ID_MAX];
  if (!copy_segment(idbuf, sizeof(idbuf), id)) {
    send_json_error(c, 404, "not_found", "Loan not found.");
    return;
  }

  double fine = 0.0;
  int overdue_days = 0;
  char errbuf[256] = {0};
  LoanReturnStatus status = loan_service_return(app ? app->db : NULL, idbuf, &fine,
                                                &overdue_days, errbuf, sizeof(errbuf));

  switch (status) {
    case LOAN_RETURN_OK:
      fprintf(stderr, "[loans] return by '%s' (%s): id=%s overdue_days=%d fine=%.2f\n",
              session->admin_id, session->role, idbuf, overdue_days, fine);
      reply_with_loan_at(app, c, 200, idbuf);
      break;
    case LOAN_RETURN_NOT_FOUND:
    case LOAN_RETURN_BAD_INPUT:
      send_json_error(c, 404, "not_found", "No issued loan with that id.");
      break;
    case LOAN_RETURN_DB_UNAVAILABLE:
      send_json_error(c, 503, "database_unavailable", "Loans are temporarily unavailable.");
      break;
    case LOAN_RETURN_QUERY_FAILED:
    default:
      fprintf(stderr, "[loans] return '%s' failed: %s\n", idbuf,
              errbuf[0] ? errbuf : "(no details)");
      send_json_error(c, 500, "database_error", "Could not return the book right now.");
      break;
  }
}