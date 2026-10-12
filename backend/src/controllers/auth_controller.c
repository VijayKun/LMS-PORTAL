/**
 * auth_controller.c - Request handling for login/logout/current-user.
 */
#include "controllers/auth_controller.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "auth/session_store.h"
#include "middleware/auth_middleware.h"
#include "middleware/cors_middleware.h"
#include "services/auth_service.h"
#include "utils/json_parse.h"
#include "utils/json_utils.h"
#include "utils/response_utils.h"

#define LOGIN_BODY_MAX 2048
#define FIELD_MAX 128

#define COOKIE_MAX (SESSION_TOKEN_STR + 128)

/* Build {"admin":{...}} with escaped field values. Returns bytes written. */
static int build_admin_json(const AuthAdmin *admin, char *out, size_t out_size) {
  char id[513], name[513], email[513], role[513], title[513];
  json_escape(admin->id, id, sizeof(id));
  json_escape(admin->name, name, sizeof(name));
  json_escape(admin->email, email, sizeof(email));
  json_escape(admin->role, role, sizeof(role));
  json_escape(admin->title, title, sizeof(title));

  return snprintf(out, out_size,
                  "{\"admin\":{\"id\":\"%s\",\"name\":\"%s\",\"email\":\"%s\","
                  "\"role\":\"%s\",\"title\":\"%s\"}}",
                  id, name, email, role, title);
}

/* Send a JSON body plus the session cookie in one reply. */
static void send_json_with_cookie(struct mg_connection *c, int status, const char *body,
                                  const char *token, long max_age, bool secure) {
  char cors[512];
  char set_cookie[COOKIE_MAX];
  char headers[1024];

  cors_headers(cors, sizeof(cors));
  snprintf(set_cookie, sizeof(set_cookie),
           "Set-Cookie: lms_session=%s; HttpOnly; Path=/; SameSite=Lax; Max-Age=%ld%s\r\n",
           token, max_age, secure ? "; Secure" : "");
  snprintf(headers, sizeof(headers), "%s%sContent-Type: application/json\r\n", cors,
           set_cookie);

  mg_http_reply(c, status, headers, "%s", (body != NULL) ? body : "{}");
}

/* Basic sanity: non-empty, not absurdly long. */
static bool field_ok(const char *s) {
  return s != NULL && s[0] != '\0' && strlen(s) < FIELD_MAX;
}

void auth_controller_login(AppContext *app, struct mg_connection *c,
                           const struct mg_http_message *hm) {
  if (hm->body.len == 0 || hm->body.len >= LOGIN_BODY_MAX) {
    send_json_error(c, 400, "invalid_body", "A JSON body is required.");
    return;
  }

  char body[LOGIN_BODY_MAX];
  memcpy(body, hm->body.buf, hm->body.len);
  body[hm->body.len] = '\0';

  char email[FIELD_MAX], password[FIELD_MAX];
  JsonParseResult r1 = json_get_string(body, "email", email, sizeof(email));
  JsonParseResult r2 = json_get_string(body, "password", password, sizeof(password));
  if (r1 != JSON_PARSE_FOUND || r2 != JSON_PARSE_FOUND ||
      !field_ok(email) || !field_ok(password)) {
    send_json_error(c, 400, "invalid_body", "email and password are required.");
    return;
  }

  char errbuf[256] = {0};
  AuthAdmin admin;
  AuthLoginStatus status =
      auth_service_login(app ? app->db : NULL, email, password, &admin, errbuf, sizeof(errbuf));

  if (status != AUTH_LOGIN_OK) {
    switch (status) {
      case AUTH_LOGIN_BAD_CREDENTIALS:
        send_json_error(c, 401, "invalid_credentials", "Invalid email or password.");
        break;
      case AUTH_LOGIN_DB_UNAVAILABLE:
        send_json_error(c, 503, "database_unavailable", "Login is temporarily unavailable.");
        break;
      case AUTH_LOGIN_QUERY_FAILED:
      default:
        fprintf(stderr, "[auth] login query failed: %s\n",
                errbuf[0] ? errbuf : "(no details)");
        send_json_error(c, 500, "database_error", "Could not check credentials right now.");
        break;
    }
    return;
  }

  if (app == NULL) {
    send_json_error(c, 500, "server_error", "Login is unavailable.");
    return;
  }

  /* Server-side session: random token, stored in memory only. */
  char token[SESSION_TOKEN_STR];
  mg_random_str(token, sizeof(token));

  time_t now = time(NULL);
  long ttl_abs = app->cfg->session_ttl_seconds > 0 ? app->cfg->session_ttl_seconds : 43200;
  long ttl_idle =
      app->cfg->session_ttl_idle_seconds > 0 ? app->cfg->session_ttl_idle_seconds : 1800;

  Session *session = session_create(&app->sessions, token, admin.id, admin.role, now,
                                    ttl_abs, ttl_idle);
  if (session == NULL) {
    send_json_error(c, 503, "too_many_sessions", "All session slots are in use.");
    return;
  }

  char response[2048];
  char inner[1024];
  build_admin_json(&admin, inner, sizeof(inner));
  snprintf(response, sizeof(response), "%s,\"expires_in\":%ld}", inner, ttl_abs);

  send_json_with_cookie(c, 200, response, session->token, ttl_abs,
                        app->cfg->cookie_secure);
  fprintf(stderr, "[auth] login ok: admin '%s' (%s)\n", admin.id, admin.role);
}

void auth_controller_logout(AppContext *app, struct mg_connection *c,
                            const struct mg_http_message *hm) {
  Session *session = auth_require(app, c, hm);
  if (session == NULL) {
    return; /* auth_require already answered 401 */
  }

  session_destroy(&app->sessions, session->token);

  char body[64];
  snprintf(body, sizeof(body), "{\"ok\":true}");
  long max_age = 0;
  send_json_with_cookie(c, 200, body, "", max_age, app->cfg->cookie_secure);
  fprintf(stderr, "[auth] logout\n");
}

void auth_controller_me(AppContext *app, struct mg_connection *c,
                        const struct mg_http_message *hm) {
  Session *session = auth_require(app, c, hm);
  if (session == NULL) {
    return; /* auth_require already answered 401 */
  }

  char errbuf[256] = {0};
  AuthAdmin admin;
  AuthMeStatus status =
      auth_service_get_admin(app ? app->db : NULL, session->admin_id, &admin, errbuf,
                             sizeof(errbuf));

  switch (status) {
    case AUTH_ME_OK: {
      char inner[1024];
      build_admin_json(&admin, inner, sizeof(inner));
      char response[1024];
      snprintf(response, sizeof(response), "%s,\"expires_in\":%ld}", inner,
               session->ttl_abs_seconds);
      send_json(c, 200, response);
      break;
    }
    case AUTH_ME_NOT_FOUND:
      /* Admin deleted: the session is meaningless, revoke it. */
      session_destroy(&app->sessions, session->token);
      send_json_error(c, 401, "unauthorized", "The session is no longer valid.");
      break;
    case AUTH_ME_DB_UNAVAILABLE:
      send_json_error(c, 503, "database_unavailable", "The user profile is unavailable.");
      break;
    case AUTH_ME_QUERY_FAILED:
    default:
      fprintf(stderr, "[auth] me query failed: %s\n",
              errbuf[0] ? errbuf : "(no details)");
      send_json_error(c, 500, "database_error", "Could not load the user profile right now.");
      break;
  }
}