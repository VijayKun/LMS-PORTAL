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

  bool credentials = strcmp(g_allowed_origin, "*") != 0;
  snprintf(buf, buf_size,
           "Access-Control-Allow-Origin: %s\r\n"
           "Access-Control-Allow-Methods: GET, POST, PUT, PATCH, DELETE, OPTIONS\r\n"
           "Access-Control-Allow-Headers: Content-Type, Authorization, X-Requested-With\r\n"
           "Access-Control-Allow-Credentials: %s\r\n"
           "Access-Control-Max-Age: 86400\r\n"
           "Vary: Origin\r\n",
           g_allowed_origin, credentials ? "true" : "false");
}

static bool method_is_safe(const struct mg_http_message *hm) {
  return mg_strcasecmp(hm->method, mg_str("GET")) == 0 ||
         mg_strcasecmp(hm->method, mg_str("HEAD")) == 0 ||
         mg_strcasecmp(hm->method, mg_str("OPTIONS")) == 0;
}

bool cors_write_allowed(const struct mg_http_message *hm) {
  if (hm == NULL || method_is_safe(hm)) {
    return true;
  }
  struct mg_str *origin = mg_http_get_header(hm, "Origin");
  if (origin == NULL || origin->len == 0) {
    return true; /* non-browser clients do not send Origin */
  }
  if (strcmp(g_allowed_origin, "*") == 0) {
    return true;
  }
  size_t allowed_len = strlen(g_allowed_origin);
  return origin->len == allowed_len &&
         memcmp(origin->buf, g_allowed_origin, allowed_len) == 0;
}

bool cors_handle_preflight(struct mg_connection *c, const struct mg_http_message *hm) {
  if (hm == NULL || mg_strcasecmp(hm->method, mg_str("OPTIONS")) != 0) {
    return false;
  }

  char headers[768];
  cors_headers(headers, sizeof(headers));
  mg_http_reply(c, 204, headers, "");
  return true;
}
