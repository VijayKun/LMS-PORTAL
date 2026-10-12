/**
 * student_routes.c - Routes for the student directory.
 */
#include "routes/student_routes.h"

#include "controllers/student_controller.h"
#include "utils/response_utils.h"

bool student_routes_handle(AppContext *app, struct mg_connection *c,
                           const struct mg_http_message *hm) {
  struct mg_str caps[1];

  bool is_item = mg_match(hm->uri, mg_str("/api/students/*"), caps);
  bool is_list = mg_match(hm->uri, mg_str("/api/students"), NULL);

  if (!is_item && !is_list) {
    return false; /* some other /api/* route */
  }

  /* GET: item then list. */
  if (mg_strcasecmp(hm->method, mg_str("GET")) == 0) {
    if (is_item) {
      student_controller_get(app, c, hm, caps[0]);
    } else {
      student_controller_list(app, c, hm);
    }
    return true;
  }

  /* Write operations require a valid session (checked inside the controller). */
  if (is_list && mg_strcasecmp(hm->method, mg_str("POST")) == 0) {
    student_controller_create(app, c, hm);
    return true;
  }
  if (is_item && mg_strcasecmp(hm->method, mg_str("PUT")) == 0) {
    student_controller_update(app, c, hm, caps[0]);
    return true;
  }
  if (is_item && mg_strcasecmp(hm->method, mg_str("DELETE")) == 0) {
    student_controller_delete(app, c, hm, caps[0]);
    return true;
  }

  return false;
}
