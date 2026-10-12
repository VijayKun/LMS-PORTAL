/**
 * book_controller.c - Request handling for the book catalog.
 *
 * Reads through the service, maps service status to an HTTP response and never
 * leaks connection strings or internal database errors to the client.
 */
#include "controllers/book_controller.h"

#include <stdio.h>
#include <stdlib.h>

#include "services/book_service.h"
#include "utils/response_utils.h"

/* Plenty for the sample catalog; larger sets would stream or paginate later. */
#define BOOKS_JSON_BUFFER_SIZE 65536

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
