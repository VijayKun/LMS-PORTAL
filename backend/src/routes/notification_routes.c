/**
 * notification_routes.c - Routes for notifications.
 */
#include "routes/notification_routes.h"

#include <string.h>

#include "controllers/notification_controller.h"
#include "utils/response_utils.h"

static bool uri_equals(const struct mg_http_message *hm, const char *path) {
  size_t len = strlen(path);
  return hm->uri.len == len && memcmp(hm->uri.buf, path, len) == 0;
}

static bool uri_starts_with(const struct mg_http_message *hm, const char *prefix) {
  size_t len = strlen(prefix);
  return hm->uri.len >= len && memcmp(hm->uri.buf, prefix, len) == 0;
}

bool notification_routes_handle(AppContext *app, struct mg_connection *c,
                                const struct mg_http_message *hm) {
  if (uri_equals(hm, "/api/notifications")) {
    if (mg_strcasecmp(hm->method, mg_str("GET")) == 0) {
      notification_controller_list(app, c, hm);
      return true;
    }
    if (mg_strcasecmp(hm->method, mg_str("POST")) == 0) {
      notification_controller_create(app, c, hm);
      return true;
    }
    send_json_error(c, 405, "method_not_allowed", "Use GET or POST.");
    return true;
  }

  if (uri_equals(hm, "/api/notifications/read-all")) {
    if (mg_strcasecmp(hm->method, mg_str("POST")) != 0) {
      send_json_error(c, 405, "method_not_allowed", "Use POST.");
      return true;
    }
    notification_controller_mark_all_read(app, c, hm);
    return true;
  }

  const char *prefix = "/api/notifications/";
  const size_t prefix_len = strlen(prefix);
  const char *suffix = "/read";
  const size_t suffix_len = strlen(suffix);
  if (hm->uri.len > prefix_len + suffix_len && uri_starts_with(hm, prefix) &&
      memcmp(hm->uri.buf + hm->uri.len - suffix_len, suffix, suffix_len) == 0) {
    if (mg_strcasecmp(hm->method, mg_str("POST")) != 0) {
      send_json_error(c, 405, "method_not_allowed", "Use POST.");
      return true;
    }
    struct mg_str id = {.buf = hm->uri.buf + prefix_len,
                        .len = hm->uri.len - prefix_len - suffix_len};
    notification_controller_mark_read(app, c, hm, id);
    return true;
  }

  if (uri_starts_with(hm, prefix)) {
    if (mg_strcasecmp(hm->method, mg_str("GET")) != 0) {
      send_json_error(c, 405, "method_not_allowed", "Use GET.");
      return true;
    }
    struct mg_str id = {.buf = hm->uri.buf + prefix_len, .len = hm->uri.len - prefix_len};
    notification_controller_get(app, c, hm, id);
    return true;
  }

  return false;
}
