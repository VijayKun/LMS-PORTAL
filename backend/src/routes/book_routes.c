/**
 * book_routes.c - Routes for the book catalog.
 */
#include "routes/book_routes.h"

#include "controllers/book_controller.h"

bool book_routes_handle(AppContext *app, struct mg_connection *c,
                        const struct mg_http_message *hm) {
  if (mg_strcasecmp(hm->method, mg_str("GET")) != 0) {
    return false;
  }
  if (!mg_match(hm->uri, mg_str("/api/books"), NULL)) {
    return false;
  }

  book_controller_list(app, c, hm);
  return true;
}
