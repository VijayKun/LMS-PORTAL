/**
 * cors_middleware.c - See cors_middleware.h.
 */
#include "middleware/cors_middleware.h"

#include <stdio.h>
#include <string.h>

static char g_allowed_origin[LMS_MAX_STR] = "*";

void cors_init(const ServerConfig *cfg) {
  if (cfg != NULL && cfg->cors_origin[0] != '\0') {
    snprintf(g_allowed_origin, sizeof(g_allowed_origin), "%s", cfg->cors_origin);
  }
}

void cors_headers(char *buf, size_t buf_size) {
  if (buf == NULL || buf_size == 0) {
    return;
  }

  snprintf(buf, buf_size,
           "Access-Control-Allow-Origin: %s\r\n"
           "Access-Control-Allow-Methods: GET, POST, PUT, PATCH, DELETE, OPTIONS\r\n"
           "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
           "Access-Control-Max-Age: 86400\r\n"
           "Vary: Origin\r\n",
           g_allowed_origin);
}

bool cors_handle_preflight(struct mg_connection *c, const struct mg_http_message *hm) {
  if (hm == NULL || mg_strcasecmp(hm->method, mg_str("OPTIONS")) != 0) {
    return false;
  }

  char headers[512];
  cors_headers(headers, sizeof(headers));
  mg_http_reply(c, 204, headers, "");
  return true;
}
