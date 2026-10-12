/**
 * auth_routes.c - Routes for authentication.
 */
#include "routes/auth_routes.h"

#include "controllers/auth_controller.h"
#include "utils/response_utils.h"

bool auth_routes_handle(AppContext *app, struct mg_connection *c,
                        const struct mg_http_message *hm) {
  /* POST /api/auth/login */
  if (mg_match(hm->uri, mg_str("/api/auth/login"), NULL)) {
    if (mg_strcasecmp(hm->method, mg_str("POST")) != 0) {
      send_json_error(c, 405, "method_not_allowed", "Use POST.");
      return true;
    }
    auth_controller_login(app, c, hm);
    return true;
  }

  /* POST /api/auth/logout */
  if (mg_match(hm->uri, mg_str("/api/auth/logout"), NULL)) {
    if (mg_strcasecmp(hm->method, mg_str("POST")) != 0) {
      send_json_error(c, 405, "method_not_allowed", "Use POST.");
      return true;
    }
    auth_controller_logout(app, c, hm);
    return true;
  }

  /* GET /api/auth/me */
  if (mg_match(hm->uri, mg_str("/api/auth/me"), NULL)) {
    if (mg_strcasecmp(hm->method, mg_str("GET")) != 0) {
      send_json_error(c, 405, "method_not_allowed", "Use GET.");
      return true;
    }
    auth_controller_me(app, c, hm);
    return true;
  }

  return false;
}