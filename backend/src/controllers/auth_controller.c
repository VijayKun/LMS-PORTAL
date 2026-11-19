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

/* Build {"user":{...}} with escaped field values. */
static int build_user_json(const AuthUser *user, char *out, size_t out_size) {
  char id[513], name[513], email[513], role[513], title[513];
  json_escape(user->id, id, sizeof(id));
  json_escape(user->name, name, sizeof(name));
  json_escape(user->email, email, sizeof(email));
  json_escape(user->role, role, sizeof(role));
  json_escape(user->title, title, sizeof(title));

  return snprintf(out, out_size,
                  "{\"user\":{\"id\":\"%s\",\"name\":\"%s\",\"email\":\"%s\","
                  "\"role\":\"%s\",\"title\":\"%s\"}}",
                  id, name, email, role, title);
}

/* Send a JSON body plus the session cookie in one reply. */
static void send_json_with_cookie(struct mg_connection *c, int status, const char *body,
                                  const char *token, long max_age, const ServerConfig *cfg) {
  char cors[768];
  char set_cookie[COOKIE_MAX];
  char headers[1200];

  cors_headers(cors, sizeof(cors));
  snprintf(set_cookie, sizeof(set_cookie),
           "Set-Cookie: lms_session=%s; HttpOnly; Path=/; SameSite=%s; Max-Age=%ld%s\r\n",
           token, cfg->cookie_same_site[0] ? cfg->cookie_same_site : "Lax", max_age,
           cfg->cookie_secure ? "; Secure" : "");
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

  char email[FIELD_MAX], password[FIELD_MAX], identifier[FIELD_MAX];
  JsonParseResult r1 = json_get_string(body, "email", email, sizeof(email));
  JsonParseResult rident = json_get_string(body, "identifier", identifier, sizeof(identifier));
  JsonParseResult r2 = json_get_string(body, "password", password, sizeof(password));

  const char *login_id = NULL;
  if (r1 == JSON_PARSE_FOUND && field_ok(email)) {
    login_id = email;
  } else if (rident == JSON_PARSE_FOUND && field_ok(identifier)) {
    login_id = identifier;
  }
  if (login_id == NULL || r2 != JSON_PARSE_FOUND || !field_ok(password)) {
    send_json_error(c, 400, "invalid_body", "email (or identifier) and password are required.");
    return;
  }

  char errbuf[256] = {0};
  AuthUser user;
  memset(&user, 0, sizeof(user));
  AuthLoginStatus status =
      auth_service_login(app ? app->db : NULL, login_id, password, &user, errbuf,
                         sizeof(errbuf));

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

  char token[SESSION_TOKEN_STR];
  mg_random_str(token, sizeof(token));

  time_t now = time(NULL);
  long ttl_abs = app->cfg->session_ttl_seconds > 0 ? app->cfg->session_ttl_seconds : 43200;
  long ttl_idle =
      app->cfg->session_ttl_idle_seconds > 0 ? app->cfg->session_ttl_idle_seconds : 1800;

  Session *session = session_create(&app->sessions, token, user.id, user.role, now, ttl_abs,
                                    ttl_idle);
  if (session == NULL) {
    send_json_error(c, 503, "too_many_sessions", "All session slots are in use.");
    return;
  }

  char user_json[1024];
  build_user_json(&user, user_json, sizeof(user_json));

  char response[1536];
  if (auth_role_is_admin(user.role)) {
    char id[513], name[513], role[513], title[513];
    json_escape(user.id, id, sizeof(id));
    json_escape(user.name, name, sizeof(name));
    json_escape(user.role, role, sizeof(role));
    json_escape(user.title, title, sizeof(title));
    snprintf(response, sizeof(response),
             "%s,\"admin\":{\"id\":\"%s\",\"name\":\"%s\",\"role\":\"%s\",\"title\":\"%s\"},"
             "\"expires_in\":%ld}",
             user_json, id, name, role, title, ttl_abs);
  } else {
    snprintf(response, sizeof(response), "%s,\"expires_in\":%ld}", user_json, ttl_abs);
  }

  send_json_with_cookie(c, 200, response, session->token, ttl_abs, app->cfg);
  fprintf(stderr, "[auth] login ok: %s '%s' (%s)\n", user.role, user.id, user.email);
}

void auth_controller_logout(AppContext *app, struct mg_connection *c,
                            const struct mg_http_message *hm) {
  Session *session = auth_require(app, c, hm);
  if (session == NULL) {
    return;
  }

  session_destroy(&app->sessions, session->token);

  char body[64];
  snprintf(body, sizeof(body), "{\"ok\":true}");
  send_json_with_cookie(c, 200, body, "", 0, app->cfg);
  fprintf(stderr, "[auth] logout\n");
}

void auth_controller_me(AppContext *app, struct mg_connection *c,
                        const struct mg_http_message *hm) {
  Session *session = auth_require(app, c, hm);
  if (session == NULL) {
    return;
  }

  char errbuf[256] = {0};
  AuthUser user;
  memset(&user, 0, sizeof(user));
  AuthMeStatus status =
      auth_service_get_user(app ? app->db : NULL, session->admin_id, session->role, &user,
                            errbuf, sizeof(errbuf));

  switch (status) {
    case AUTH_ME_OK: {
      char user_json[1024];
      build_user_json(&user, user_json, sizeof(user_json));
      char response[1280];
      snprintf(response, sizeof(response), "%s,\"expires_in\":%ld}", user_json,
               session->ttl_abs_seconds);
      send_json(c, 200, response);
      break;
    }
    case AUTH_ME_NOT_FOUND:
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
