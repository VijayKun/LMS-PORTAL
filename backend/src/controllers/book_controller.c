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

/* Plenty for the sample catalog; larger sets would stream or paginate later. */
#define BOOKS_JSON_BUFFER_SIZE 65536

/* A single book is small; this is generous for the selected columns. */
#define BOOK_JSON_BUFFER_SIZE 8192

/* Longest id we accept before answering 404 without touching the database. */
#define BOOK_ID_MAX 128

void book_controller_list(AppContext *app, struct mg_connection *c,
                          const struct mg_http_message *hm) {
  (void)hm; /* no query parameters yet */

  char *buffer = malloc(BOOKS_JSON_BUFFER_SIZE);
  if (buffer == NULL) {
    send_json_error(c, 500, "server_error", "The server ran out of memory.");
    return;
  }

  char errbuf[256] = {0};
  BookListStatus status =
      book_service_list(app ? app->db : NULL, buffer, BOOKS_JSON_BUFFER_SIZE, errbuf,
                        sizeof(errbuf));

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

void book_controller_get(AppContext *app, struct mg_connection *c,
                         const struct mg_http_message *hm, struct mg_str id) {
  (void)hm;

  /* Copy the URI slice into a NUL-terminated buffer (hm->uri is not one). */
  if (id.len == 0 || id.len >= BOOK_ID_MAX) {
    send_json_error(c, 404, "not_found", "Book not found.");
    return;
  }
  char idbuf[BOOK_ID_MAX];
  memcpy(idbuf, id.buf, id.len);
  idbuf[id.len] = '\0';

  char *buffer = malloc(BOOK_JSON_BUFFER_SIZE);
  if (buffer == NULL) {
    send_json_error(c, 500, "server_error", "The server ran out of memory.");
    return;
  }

  char errbuf[256] = {0};
  BookGetStatus status = book_service_get(app ? app->db : NULL, idbuf, buffer,
                                          BOOK_JSON_BUFFER_SIZE, errbuf, sizeof(errbuf));

  switch (status) {
    case BOOK_GET_OK:
      send_json(c, 200, buffer);
      break;

    case BOOK_GET_NOT_FOUND:
      send_json_error(c, 404, "not_found", "Book not found.");
      break;

    case BOOK_GET_DB_UNAVAILABLE:
      send_json_error(c, 503, "database_unavailable",
                      "The book catalog is temporarily unavailable.");
      break;

    case BOOK_GET_BUFFER_TOO_SMALL:
      fprintf(stderr, "[books] response buffer too small for book '%s'\n", idbuf);
      send_json_error(c, 500, "response_too_large", "The book could not be returned.");
      break;

    case BOOK_GET_QUERY_FAILED:
    default:
      fprintf(stderr, "[books] get '%s' failed: %s\n", idbuf,
              errbuf[0] ? errbuf : "(no details)");
      send_json_error(c, 500, "database_error", "Could not load the book right now.");
      break;
  }

  free(buffer);
}
