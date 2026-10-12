/**
 * health_routes.c - Liveness endpoint, no database access.
 */
#include "routes/health_routes.h"

#include <stdio.h>
#include <time.h>

#include "config/config.h"
#include "utils/response_utils.h"

static time_t g_started_at = 0;

void health_routes_init(void) {
  g_started_at = time(NULL);
}

bool health_routes_handle(AppContext *app, struct mg_connection *c,
                          const struct mg_http_message *hm) {
  (void)app; /* health does not need the database */

  if (mg_strcasecmp(hm->method, mg_str("GET")) != 0) {
    return false;
  }
  if (!mg_match(hm->uri, mg_str("/api/health"), NULL)) {
    return false;
  }

  long uptime = (long)(time(NULL) - g_started_at);
  if (uptime < 0) {
    uptime = 0;
  }

  char body[256];
  snprintf(body, sizeof(body),
           "{\"status\":\"ok\",\"service\":\"%s\",\"version\":\"%s\",\"uptime_seconds\":%ld}",
           LMS_BACKEND_NAME, LMS_BACKEND_VERSION, uptime);

  send_json(c, 200, body);
  return true;
}
