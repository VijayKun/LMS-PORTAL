/**
 * student_routes.c - Routes for the student directory.
 */
#include "routes/student_routes.h"

#include "controllers/student_controller.h"

bool student_routes_handle(AppContext *app, struct mg_connection *c,
                           const struct mg_http_message *hm) {
  if (mg_strcasecmp(hm->method, mg_str("GET")) != 0) {
    return false;
  }

  /* GET /api/students/:id - the '*' captures exactly one path segment. */
  struct mg_str caps[1];
  if (mg_match(hm->uri, mg_str("/api/students/*"), caps)) {
    student_controller_get(app, c, hm, caps[0]);
    return true;
  }

  if (!mg_match(hm->uri, mg_str("/api/students"), NULL)) {
    return false;
  }

  student_controller_list(app, c, hm);
  return true;
}
