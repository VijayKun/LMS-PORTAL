/**
 * loan_routes.c - Routes for loans/transactions.
 *
 * The canonical paths are /api/transactions (matching the frontend contract).
 * /api/loans is kept as a read-only alias for backwards compatibility.
 */
#include "routes/loan_routes.h"

#include "controllers/loan_controller.h"
#include "utils/response_utils.h"

static bool method_is(const struct mg_http_message *hm, const char *m) {
  return mg_strcasecmp(hm->method, mg_str(m)) == 0;
}

bool loan_routes_handle(AppContext *app, struct mg_connection *c,
                        const struct mg_http_message *hm) {
  struct mg_str caps[1];

  /* POST /api/transactions/:id/return - checked before the generic item path. */
  if (mg_match(hm->uri, mg_str("/api/transactions/*/return"), caps)) {
    if (!method_is(hm, "POST")) {
      send_json_error(c, 405, "method_not_allowed", "Use POST.");
      return true;
    }
    loan_controller_return(app, c, hm, caps[0]);
    return true;
  }

  /* Canonical /api/transactions paths. */
  bool is_item = mg_match(hm->uri, mg_str("/api/transactions/*"), caps);
  bool is_list = mg_match(hm->uri, mg_str("/api/transactions"), NULL);

  if (!is_item && !is_list) {
    /* Legacy read-only /api/loans alias (GET only). */
    if (mg_match(hm->uri, mg_str("/api/loans"), NULL)) {
      if (!method_is(hm, "GET")) {
        return false;
      }
      loan_controller_list(app, c, hm);
      return true;
    }
    if (mg_match(hm->uri, mg_str("/api/loans/*"), caps)) {
      if (!method_is(hm, "GET")) {
        return false;
      }
      loan_controller_get(app, c, hm, caps[0]);
      return true;
    }
    return false; /* some other /api/* route */
  }

  if (method_is(hm, "GET")) {
    if (is_item) {
      loan_controller_get(app, c, hm, caps[0]);
    } else {
      loan_controller_list(app, c, hm);
    }
    return true;
  }

  /* Writes require a valid session (checked inside the controller). */
  if (is_list && method_is(hm, "POST")) {
    loan_controller_issue(app, c, hm);
    return true;
  }

  return false;
}
