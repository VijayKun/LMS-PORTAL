/**
 * auth_controller.h - Request handling for login/logout/current-user.
 */
#ifndef LMS_AUTH_CONTROLLER_H
#define LMS_AUTH_CONTROLLER_H

#include "mongoose.h"
#include "server/server.h"

/** POST /api/auth/login - verify credentials and mint a session cookie. */
void auth_controller_login(AppContext *app, struct mg_connection *c,
                           const struct mg_http_message *hm);

/** POST /api/auth/logout - destroy the current session (requires auth). */
void auth_controller_logout(AppContext *app, struct mg_connection *c,
                            const struct mg_http_message *hm);

/** GET /api/auth/me - return the currently logged-in admin (requires auth). */
void auth_controller_me(AppContext *app, struct mg_connection *c,
                        const struct mg_http_message *hm);

#endif /* LMS_AUTH_CONTROLLER_H */