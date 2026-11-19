/**
 * branch_routes.c - Routes for library branches.
 */
#include "routes/branch_routes.h"

#include "controllers/branch_controller.h"
#include "utils/response_utils.h"

bool branch_routes_handle(AppContext *app, struct mg_connection *c,
                          const struct mg_http_message *hm) {
  struct mg_str caps[1];
  bool is_item = mg_match(hm->uri, mg_str("/api/branches/*"), caps);
  bool is_list = mg_match(hm->uri, mg_str("/api/branches"), NULL);
  if (!is_item && !is_list) {
    return false;
  }

  if (is_item) {
    if (mg_strcasecmp(hm->method, mg_str("GET")) == 0) {
      send_json_error(c, 404, "not_found", "Use the branch list endpoint.");
      return true;
    }
    if (mg_strcasecmp(hm->method, mg_str("PUT")) == 0) {
      branch_controller_update(app, c, hm, caps[0]);
      return true;
    }
    if (mg_strcasecmp(hm->method, mg_str("DELETE")) == 0) {
      branch_controller_delete(app, c, hm, caps[0]);
      return true;
    }
    send_json_error(c, 405, "method_not_allowed", "Use PUT or DELETE.");
    return true;
  }

  if (mg_strcasecmp(hm->method, mg_str("GET")) == 0) {
    branch_controller_list(app, c, hm);
    return true;
  }
  if (mg_strcasecmp(hm->method, mg_str("POST")) == 0) {
    branch_controller_create(app, c, hm);
    return true;
  }
  send_json_error(c, 405, "method_not_allowed", "Use GET or POST.");
  return true;
}
