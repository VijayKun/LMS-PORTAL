/**
 * report_routes.c - Routes for reports.
 */
#include "routes/report_routes.h"

#include "controllers/report_controller.h"
#include "utils/response_utils.h"

bool report_routes_handle(AppContext *app, struct mg_connection *c,
                          const struct mg_http_message *hm) {
  struct mg_str caps[1];
  if (!mg_match(hm->uri, mg_str("/api/reports/*"), caps)) {
    return false;
  }

  if (mg_strcasecmp(hm->method, mg_str("GET")) != 0) {
    send_json_error(c, 405, "method_not_allowed", "Use GET.");
    return true;
  }

  report_controller_get(app, c, hm, caps[0]);
  return true;
}
