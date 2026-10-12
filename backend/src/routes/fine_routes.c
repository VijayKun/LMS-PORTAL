/**
 * fine_routes.c - Routes for fines.
 */
#include "routes/fine_routes.h"

#include "controllers/fine_controller.h"
#include "utils/response_utils.h"

static bool method_is(const struct mg_http_message *hm, const char *m) {
  return mg_strcasecmp(hm->method, mg_str(m)) == 0;
}

bool fine_routes_handle(AppContext *app, struct mg_connection *c,
                        const struct mg_http_message *hm) {
  struct mg_str caps[1];

  /* POST /api/fines/:id/pay - before the generic item path. */
  if (mg_match(hm->uri, mg_str("/api/fines/*/pay"), caps)) {
    if (!method_is(hm, "POST")) {
      send_json_error(c, 405, "method_not_allowed", "Use POST.");
      return true;
    }
    fine_controller_pay(app, c, hm, caps[0]);
    return true;
  }

  bool is_item = mg_match(hm->uri, mg_str("/api/fines/*"), caps);
  bool is_list = mg_match(hm->uri, mg_str("/api/fines"), NULL);

  if (!is_item && !is_list) {
    return false; /* some other /api/* route */
  }

  if (!method_is(hm, "GET")) {
    return false;
  }

  if (is_item) {
    fine_controller_get(app, c, hm, caps[0]);
  } else {
    fine_controller_list(app, c, hm);
  }
  return true;
}
