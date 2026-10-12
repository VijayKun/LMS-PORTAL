/**
 * reservation_routes.c - Routes for reservations.
 */
#include "routes/reservation_routes.h"

#include "controllers/reservation_controller.h"
#include "utils/response_utils.h"

static bool method_is(const struct mg_http_message *hm, const char *m) {
  return mg_strcasecmp(hm->method, mg_str(m)) == 0;
}

bool reservation_routes_handle(AppContext *app, struct mg_connection *c,
                               const struct mg_http_message *hm) {
  struct mg_str caps[1];

  /* POST /api/reservations/:id/ready - before the generic item path. */
  if (mg_match(hm->uri, mg_str("/api/reservations/*/ready"), caps)) {
    if (!method_is(hm, "POST")) {
      send_json_error(c, 405, "method_not_allowed", "Use POST.");
      return true;
    }
    reservation_controller_ready(app, c, hm, caps[0]);
    return true;
  }

  bool is_item = mg_match(hm->uri, mg_str("/api/reservations/*"), caps);
  bool is_list = mg_match(hm->uri, mg_str("/api/reservations"), NULL);

  if (!is_item && !is_list) {
    return false; /* some other /api/* route */
  }

  if (method_is(hm, "GET")) {
    if (is_item) {
      reservation_controller_get(app, c, hm, caps[0]);
    } else {
      reservation_controller_list(app, c, hm);
    }
    return true;
  }

  /* Writes require a valid session (checked inside the controller). */
  if (is_list && method_is(hm, "POST")) {
    reservation_controller_create(app, c, hm);
    return true;
  }
  if (is_item && method_is(hm, "DELETE")) {
    reservation_controller_cancel(app, c, hm, caps[0]);
    return true;
  }

  return false;
}
