/**
 * book_controller.c - Request handling for the book catalog.
 *
 * Reads through the service, maps service status to an HTTP response and never
 * leaks connection strings or internal database errors to the client.
 */
#include "controllers/book_controller.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "middleware/auth_middleware.h"
#include "services/book_service.h"
#include "utils/json_parse.h"
#include "utils/response_utils.h"

/* Large enough for a full page of the catalog (bounded by the limit cap). */
#define BOOKS_JSON_BUFFER_SIZE 262144

/* A single book is small; this is generous for the selected columns. */
#define BOOK_JSON_BUFFER_SIZE 8192

/* Longest id/isbn value we accept before answering 404 without touching the DB. */
#define BOOK_ID_MAX 128

/* Upper bound for a create/update request body. */
#define BOOK_BODY_MAX 8192

/* Emit the response for a single-book lookup (shared by id and isbn). */
static void send_get_status(struct mg_connection *c, BookGetStatus status,
                            const char *value, const char *errbuf) {
  switch (status) {
    case BOOK_GET_OK:
      return; /* caller sends the body */
    case BOOK_GET_NOT_FOUND:
      send_json_error(c, 404, "not_found", "Book not found.");
      break;
    case BOOK_GET_DB_UNAVAILABLE:
      send_json_error(c, 503, "database_unavailable",
                      "The book catalog is temporarily unavailable.");
      break;
    case BOOK_GET_BUFFER_TOO_SMALL:
      fprintf(stderr, "[books] response buffer too small for '%s'\n", value);
      send_json_error(c, 500, "response_too_large", "The book could not be returned.");
      break;
    case BOOK_GET_QUERY_FAILED:
    default:
      fprintf(stderr, "[books] get '%s' failed: %s\n", value,
              errbuf != NULL && errbuf[0] ? errbuf : "(no details)");
      send_json_error(c, 500, "database_error", "Could not load the book right now.");
      break;
  }
}

void book_controller_list(AppContext *app, struct mg_connection *c,
                          const struct mg_http_message *hm) {
  BookListQuery query = {0};
  char qbuf[128];
  char catbuf[128];
  char numbuf[16];

  if (mg_http_get_var(&hm->query, "q", qbuf, sizeof(qbuf)) >= 0) {
    query.q = qbuf;
  }
  if (mg_http_get_var(&hm->query, "category", catbuf, sizeof(catbuf)) >= 0) {
    query.category = catbuf;
  }
  if (mg_http_get_var(&hm->query, "limit", numbuf, sizeof(numbuf)) >= 0) {
    query.limit = (int)strtol(numbuf, NULL, 10);
  }
  if (mg_http_get_var(&hm->query, "offset", numbuf, sizeof(numbuf)) >= 0) {
    query.offset = (int)strtol(numbuf, NULL, 10);
  }

  char *buffer = malloc(BOOKS_JSON_BUFFER_SIZE);
  if (buffer == NULL) {
    send_json_error(c, 500, "server_error", "The server ran out of memory.");
    return;
  }

  char errbuf[256] = {0};
  BookListStatus status = book_service_search(
      app ? app->db : NULL, &query, buffer, BOOKS_JSON_BUFFER_SIZE, errbuf, sizeof(errbuf));

  switch (status) {
    case BOOK_LIST_OK:
      send_json(c, 200, buffer);
      break;

    case BOOK_LIST_DB_UNAVAILABLE:
      send_json_error(c, 503, "database_unavailable",
                      "The book catalog is temporarily unavailable.");
      break;

    case BOOK_LIST_BUFFER_TOO_SMALL:
      fprintf(stderr, "[books] response buffer too small for the result set\n");
      send_json_error(c, 500, "response_too_large",
                      "The book list is too large to return.");
      break;

    case BOOK_LIST_QUERY_FAILED:
    default:
      /* Log server-side only; the client gets a generic message. */
      fprintf(stderr, "[books] query failed: %s\n", errbuf[0] ? errbuf : "(no details)");
      send_json_error(c, 500, "database_error", "Could not load books right now.");
      break;
  }

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

void book_controller_get(AppContext *app, struct mg_connection *c,
                         const struct mg_http_message *hm, struct mg_str id) {
  (void)hm;

  char idbuf[BOOK_ID_MAX];
  if (!copy_segment(idbuf, sizeof(idbuf), id)) {
    send_json_error(c, 404, "not_found", "Book not found.");
    return;
  }

  char *buffer = malloc(BOOK_JSON_BUFFER_SIZE);
  if (buffer == NULL) {
    send_json_error(c, 500, "server_error", "The server ran out of memory.");
    return;
  }

  char errbuf[256] = {0};
  BookGetStatus status = book_service_get(app ? app->db : NULL, idbuf, buffer,
                                          BOOK_JSON_BUFFER_SIZE, errbuf, sizeof(errbuf));

  if (status == BOOK_GET_OK) {
    send_json(c, 200, buffer);
  } else {
    send_get_status(c, status, idbuf, errbuf);
  }

  free(buffer);
}

void book_controller_get_by_isbn(AppContext *app, struct mg_connection *c,
                                 const struct mg_http_message *hm, struct mg_str isbn) {
  (void)hm;

  char isbnbuf[BOOK_ID_MAX];
  if (!copy_segment(isbnbuf, sizeof(isbnbuf), isbn)) {
    send_json_error(c, 404, "not_found", "Book not found.");
    return;
  }

  char *buffer = malloc(BOOK_JSON_BUFFER_SIZE);
  if (buffer == NULL) {
    send_json_error(c, 500, "server_error", "The server ran out of memory.");
    return;
  }

  char errbuf[256] = {0};
  BookGetStatus status = book_service_get_by_isbn(app ? app->db : NULL, isbnbuf, buffer,
                                                  BOOK_JSON_BUFFER_SIZE, errbuf,
                                                  sizeof(errbuf));

  if (status == BOOK_GET_OK) {
    send_json(c, 200, buffer);
  } else {
    send_get_status(c, status, isbnbuf, errbuf);
  }

  free(buffer);
}

/* ---- Create / update / delete ------------------------------------------ */

/*
 * Read the JSON body into a zeroed BookMutate. Fields that are absent stay
 * empty ("not provided"); a present field copies into the fixed-size buffer,
 * so oversized values fail here with 400 instead of overflowing anything.
 */
static bool parse_book_body(AppContext *app, struct mg_connection *c,
                            const struct mg_http_message *hm, BookMutate *out) {
  if (hm->body.len == 0 || hm->body.len >= BOOK_BODY_MAX) {
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

  char body[BOOK_BODY_MAX];
  memcpy(body, hm->body.buf, hm->body.len);
  body[hm->body.len] = '\0';

  memset(out, 0, sizeof(*out));

  struct {
    const char *key;
    char *dst;
    size_t size;
  } fields[] = {
      {"title", out->title, sizeof(out->title)},
      {"author", out->author, sizeof(out->author)},
      {"isbn", out->isbn, sizeof(out->isbn)},
      {"category", out->category, sizeof(out->category)},
      {"publisher", out->publisher, sizeof(out->publisher)},
      {"publication_date", out->publication_date, sizeof(out->publication_date)},
      {"description", out->description, sizeof(out->description)},
      {"cover", out->cover, sizeof(out->cover)},
      {"branch_id", out->branch_id, sizeof(out->branch_id)},
      {"total_copies", out->total_copies, sizeof(out->total_copies)},
      {"available_copies", out->available_copies, sizeof(out->available_copies)},
      {"year", out->year, sizeof(out->year)},
  };

  for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); i++) {
    JsonParseResult r = json_get_string(body, fields[i].key, fields[i].dst, fields[i].size);
    if (r == JSON_PARSE_ERROR) {
      send_json_error(c, 400, "invalid_body", "The request body could not be parsed.");
      return false;
    }
    /* JSON_PARSE_NOT_FOUND: leave the field empty (not provided). */
  }

  (void)app;
  return true;
}

/* Load the created/updated row and reply 2xx with the fresh book object. */
static void reply_with_book(AppContext *app, struct mg_connection *c, int status_code,
                            const char *idbuf) {
  char *buffer = malloc(BOOK_JSON_BUFFER_SIZE);
  if (buffer == NULL) {
    send_json_error(c, 500, "server_error", "The server ran out of memory.");
    return;
  }

  char errbuf[256] = {0};
  BookGetStatus gs = book_service_get(app ? app->db : NULL, idbuf, buffer,
                                      BOOK_JSON_BUFFER_SIZE, errbuf, sizeof(errbuf));
  if (gs == BOOK_GET_OK) {
    send_json(c, status_code, buffer);
  } else {
    send_get_status(c, gs, idbuf, errbuf);
  }
  free(buffer);
}

void book_controller_create(AppContext *app, struct mg_connection *c,
                            const struct mg_http_message *hm) {
  Session *session = auth_require_admin(app, c, hm);
  if (session == NULL) {
    return; /* auth_require already answered 401 */
  }

  BookMutate book;
  if (!parse_book_body(app, c, hm, &book)) {
    return;
  }

  char idbuf[BOOK_ID_MAX];
  char errbuf[256] = {0};
  BookCreateStatus status = book_service_create(app ? app->db : NULL, &book, idbuf,
                                                sizeof(idbuf), errbuf, sizeof(errbuf));

  switch (status) {
    case BOOK_CREATE_OK:
      fprintf(stderr, "[books] create by '%s' (%s): id=%s\n", session->admin_id,
              session->role, idbuf);
      reply_with_book(app, c, 201, idbuf);
      break;
    case BOOK_CREATE_BAD_INPUT:
      send_json_error(c, 400, "invalid_body",
                      "Title and ISBN are required; counts and year must be valid.");
      break;
    case BOOK_CREATE_DUPLICATE:
      send_json_error(c, 409, "conflict", "A book with this ISBN already exists.");
      break;
    case BOOK_CREATE_DB_UNAVAILABLE:
      send_json_error(c, 503, "database_unavailable", "The book catalog is temporarily unavailable.");
      break;
    case BOOK_CREATE_QUERY_FAILED:
    default:
      fprintf(stderr, "[books] create failed: %s\n", errbuf[0] ? errbuf : "(no details)");
      send_json_error(c, 500, "database_error", "Could not create the book right now.");
      break;
  }
}

void book_controller_update(AppContext *app, struct mg_connection *c,
                            const struct mg_http_message *hm, struct mg_str id) {
  Session *session = auth_require_admin(app, c, hm);
  if (session == NULL) {
    return; /* auth_require already answered 401 */
  }

  char idbuf[BOOK_ID_MAX];
  if (!copy_segment(idbuf, sizeof(idbuf), id)) {
    send_json_error(c, 404, "not_found", "Book not found.");
    return;
  }

  BookMutate book;
  if (!parse_book_body(app, c, hm, &book)) {
    return;
  }

  char errbuf[256] = {0};
  BookUpdateStatus status =
      book_service_update(app ? app->db : NULL, idbuf, &book, errbuf, sizeof(errbuf));

  switch (status) {
    case BOOK_UPDATE_OK:
      fprintf(stderr, "[books] update by '%s' (%s): id=%s\n", session->admin_id,
              session->role, idbuf);
      reply_with_book(app, c, 200, idbuf);
      break;
    case BOOK_UPDATE_NOT_FOUND:
      send_json_error(c, 404, "not_found", "Book not found.");
      break;
    case BOOK_UPDATE_BAD_INPUT:
      send_json_error(c, 400, "invalid_body",
                      "At least one editable field is required and values must be valid.");
      break;
    case BOOK_UPDATE_DUPLICATE:
      send_json_error(c, 409, "conflict", "A book with this ISBN already exists.");
      break;
    case BOOK_UPDATE_DB_UNAVAILABLE:
      send_json_error(c, 503, "database_unavailable", "The book catalog is temporarily unavailable.");
      break;
    case BOOK_UPDATE_QUERY_FAILED:
    default:
      fprintf(stderr, "[books] update '%s' failed: %s\n", idbuf,
              errbuf[0] ? errbuf : "(no details)");
      send_json_error(c, 500, "database_error", "Could not update the book right now.");
      break;
  }
}

void book_controller_delete(AppContext *app, struct mg_connection *c,
                            const struct mg_http_message *hm, struct mg_str id) {
  Session *session = auth_require_admin(app, c, hm);
  if (session == NULL) {
    return; /* auth_require already answered 401 */
  }

  char idbuf[BOOK_ID_MAX];
  if (!copy_segment(idbuf, sizeof(idbuf), id)) {
    send_json_error(c, 404, "not_found", "Book not found.");
    return;
  }

  char errbuf[256] = {0};
  BookDeleteStatus status =
      book_service_delete(app ? app->db : NULL, idbuf, errbuf, sizeof(errbuf));

  switch (status) {
    case BOOK_DELETE_OK:
      fprintf(stderr, "[books] delete by '%s' (%s): id=%s\n", session->admin_id,
              session->role, idbuf);
      send_json(c, 200, "{\"ok\":true}");
      break;
    case BOOK_DELETE_NOT_FOUND:
      send_json_error(c, 404, "not_found", "Book not found.");
      break;
    case BOOK_DELETE_IN_USE:
      send_json_error(c, 409, "conflict",
                      "This book is referenced by loans and cannot be deleted.");
      break;
    case BOOK_DELETE_DB_UNAVAILABLE:
      send_json_error(c, 503, "database_unavailable", "The book catalog is temporarily unavailable.");
      break;
    case BOOK_DELETE_QUERY_FAILED:
    default:
      fprintf(stderr, "[books] delete '%s' failed: %s\n", idbuf,
              errbuf[0] ? errbuf : "(no details)");
      send_json_error(c, 500, "database_error", "Could not delete the book right now.");
      break;
  }
}