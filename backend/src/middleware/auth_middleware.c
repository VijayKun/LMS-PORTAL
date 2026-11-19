/**
 * auth_middleware.c - See auth_middleware.h.
 */
#include "middleware/auth_middleware.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "utils/response_utils.h"

#define COOKIE_NAME "lms_session="

static void trim_spaces(char *s) {
  size_t len = strlen(s);
  while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' || s[len - 1] == '\r')) {
    s[--len] = '\0';
  }
}

static bool copy_token_value(const char *start, size_t max_len, char *out,
                             size_t out_size) {
  /* Value ends at the next ';', whitespace or end of header. */
  size_t n = 0;
  while (n < max_len && start[n] != '\0' && start[n] != ';' && start[n] != ' ' &&
         start[n] != '\r' && start[n] != '\n') {
    n++;
  }
  if (n == 0 || n >= out_size) {
    return false;
  }
  memcpy(out, start, n);
  out[n] = '\0';
  return true;
}

bool auth_extract_token(const struct mg_http_message *hm, char *out, size_t out_size) {
  if (hm == NULL || out == NULL || out_size == 0) {
    return false;
  }
  out[0] = '\0';

  /* Prefer an explicit Authorization: Bearer ... header. */
  struct mg_str *auth = mg_http_get_header(hm, "Authorization");
  if (auth != NULL && auth->len > 7 && memcmp(auth->buf, "Bearer ", 7) == 0) {
    char *tok = auth->buf + 7;
    size_t len = auth->len - 7;
    if (copy_token_value(tok, len, out, out_size)) {
      return true;
    }
  }

  /* Fall back to the HttpOnly cookie. */
  struct mg_str *cookie = mg_http_get_header(hm, "Cookie");
  if (cookie == NULL) {
    return false;
  }

  /* Look for the cookie name inside the header value. */
  const size_t name_len = strlen(COOKIE_NAME);
  const char *pos = cookie->buf;
  const char *end = cookie->buf + cookie->len;
  while (pos + name_len <= end) {
    if (memcmp(pos, COOKIE_NAME, name_len) == 0) {
      return copy_token_value(pos + name_len, (size_t)(end - (pos + name_len)), out,
                              out_size);
    }
    pos++;
  }
  return false;
}

Session *auth_require(AppContext *app, struct mg_connection *c,
                      const struct mg_http_message *hm) {
  if (app == NULL || hm == NULL) {
    send_json_error(c, 401, "unauthorized", "Authentication required.");
    return NULL;
  }

  char token[SESSION_TOKEN_STR];
  if (!auth_extract_token(hm, token, sizeof(token))) {
    send_json_error(c, 401, "unauthorized", "Authentication required.");
    return NULL;
  }

  time_t now = time(NULL);
  session_purge_expired(&app->sessions, now);

  Session *session = session_lookup(&app->sessions, token, now);
  if (session == NULL) {
    send_json_error(c, 401, "unauthorized", "The session is invalid or has expired.");
    return NULL;
  }
  return session;
}
bool auth_role_is_admin(const char *role) {
  return role != NULL && (strcmp(role, "admin") == 0 || strcmp(role, "librarian") == 0);
}

Session *auth_require_admin(AppContext *app, struct mg_connection *c,
                            const struct mg_http_message *hm) {
  Session *session = auth_require(app, c, hm);
  if (session == NULL) {
    return NULL;
  }
  if (!auth_role_is_admin(session->role)) {
    send_json_error(c, 403, "forbidden", "Administrator access is required.");
    return NULL;
  }
  return session;
}
