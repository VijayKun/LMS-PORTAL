/**
 * auth_routes.h - Routes for authentication.
 */
#ifndef LMS_AUTH_ROUTES_H
#define LMS_AUTH_ROUTES_H

#include <stdbool.h>

#include "mongoose.h"
#include "server/server.h"

/**
 * Handle POST /api/auth/login, POST /api/auth/logout and GET /api/auth/me.
 * @return true when the request was handled here.
 */
bool auth_routes_handle(AppContext *app, struct mg_connection *c,
                        const struct mg_http_message *hm);

#endif /* LMS_AUTH_ROUTES_H */