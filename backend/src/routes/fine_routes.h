/**
 * fine_routes.h - Routes for fines.
 */
#ifndef LMS_FINE_ROUTES_H
#define LMS_FINE_ROUTES_H

#include <stdbool.h>

#include "mongoose.h"
#include "server/server.h"

/**
 * Handle GET /api/fines and GET /api/fines/:id.
 * @return true when the request was handled here.
 */
bool fine_routes_handle(AppContext *app, struct mg_connection *c,
                        const struct mg_http_message *hm);

#endif /* LMS_FINE_ROUTES_H */