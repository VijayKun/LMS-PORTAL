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

#include "services/book_service.h"
#include "utils/response_utils.h"

/* Large enough for a full page of the catalog (bounded by the limit cap). */
#define BOOKS_JSON_BUFFER_SIZE 262144

/* A single book is small; this is generous for the selected columns. */
#define BOOK_JSON_BUFFER_SIZE 8192

/* Longest id/isbn value we accept before answering 404 without touching the DB. */
#define BOOK_ID_MAX 128

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