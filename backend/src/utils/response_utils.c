/**
 * response_utils.c - See response_utils.h.
 */
#include "utils/response_utils.h"

#include <stdio.h>

#include "middleware/cors_middleware.h"
#include "utils/json_utils.h"

void send_json(struct mg_connection *c, int status, const char *body) {
  char cors[512];
  char headers[640];

  cors_headers(cors, sizeof(cors));

  /* Combined header block: CORS headers + JSON content type. */
  snprintf(headers, sizeof(headers), "%sContent-Type: application/json\r\n", cors);

  mg_http_reply(c, status, headers, "%s", (body != NULL) ? body : "{}");
}

void send_json_error(struct mg_connection *c, int status, const char *code,
                     const char *message) {
  char esc_code[128];
  char esc_message[512];
  char body[768];

  json_escape((code != NULL) ? code : "error", esc_code, sizeof(esc_code));
  json_escape((message != NULL) ? message : "", esc_message, sizeof(esc_message));

  snprintf(body, sizeof(body), "{\"error\":\"%s\",\"message\":\"%s\"}", esc_code,
           esc_message);

  send_json(c, status, body);
}
