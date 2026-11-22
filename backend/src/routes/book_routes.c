/**
 * book_routes.c - Routes for the book catalog.
 */
#include "routes/book_routes.h"

#include "controllers/book_controller.h"
#include "utils/response_utils.h"

bool book_routes_handle(AppContext *app, struct mg_connection *c,
                        const struct mg_http_message *hm) {
  struct mg_str caps[1];

  /* POST /api/books/import - bulk create books (admin only). Must be first since */
  /* /api/books/* would otherwise match this as an item route. */
  if (mg_match(hm->uri, mg_str("/api/books/import"), NULL)) {
    if (mg_strcasecmp(hm->method, mg_str("POST")) != 0) {
      send_json_error(c, 405, "method_not_allowed", "Use POST.");
      return true;
    }
    book_controller_import(app, c, hm);
    return true;
  }

  /* Google Books metadata lookup - checked before the isbn/id routes. */
  if (mg_match(hm->uri, mg_str("/api/books/lookup/*"), caps)) {
    if (mg_strcasecmp(hm->method, mg_str("GET")) != 0) {
      send_json_error(c, 405, "method_not_allowed", "Use GET.");
      return true;
    }
    book_controller_lookup_isbn(app, c, hm, caps[0]);
    return true;
  }

  /* GET /api/books/isbn/:isbn - checked first so it is not read as an id. */
  if (mg_match(hm->uri, mg_str("/api/books/isbn/*"), caps)) {
    if (mg_strcasecmp(hm->method, mg_str("GET")) != 0) {
      send_json_error(c, 405, "method_not_allowed", "Use GET.");
      return true;
    }
    book_controller_get_by_isbn(app, c, hm, caps[0]);
    return true;
  }

  bool is_item = mg_match(hm->uri, mg_str("/api/books/*"), caps);
  bool is_list = mg_match(hm->uri, mg_str("/api/books"), NULL);

  if (!is_item && !is_list) {
    return false; /* some other /api/* route */
  }

  /* GET: item then list. */
  if (mg_strcasecmp(hm->method, mg_str("GET")) == 0) {
    if (is_item) {
      book_controller_get(app, c, hm, caps[0]);
    } else {
      book_controller_list(app, c, hm);
    }
    return true;
  }

  /* Write operations require a valid session (checked inside the controller). */
  if (is_list && mg_strcasecmp(hm->method, mg_str("POST")) == 0) {
    book_controller_create(app, c, hm);
    return true;
  }
  if (is_item && mg_strcasecmp(hm->method, mg_str("PUT")) == 0) {
    book_controller_update(app, c, hm, caps[0]);
    return true;
  }
  if (is_item && mg_strcasecmp(hm->method, mg_str("DELETE")) == 0) {
    book_controller_delete(app, c, hm, caps[0]);
    return true;
  }

  return false;
}
