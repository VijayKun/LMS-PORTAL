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

  /* GET /api/books/isbn/:isbn - checked first so it is not read as an id. */
  struct mg_str caps[1];
  if (mg_match(hm->uri, mg_str("/api/books/isbn/*"), caps)) {
    book_controller_get_by_isbn(app, c, hm, caps[0]);
    return true;
  }

  /* GET /api/books/:id - the '*' captures exactly one path segment. */
  if (mg_match(hm->uri, mg_str("/api/books/*"), caps)) {
    book_controller_get(app, c, hm, caps[0]);
    return true;
  }

  if (!mg_match(hm->uri, mg_str("/api/books"), NULL)) {
    return false;
  }

  book_controller_list(app, c, hm);
  return true;
}
