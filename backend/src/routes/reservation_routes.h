/**
 * reservation_routes.h - Routes for reservations.
 */
#ifndef LMS_RESERVATION_ROUTES_H
#define LMS_RESERVATION_ROUTES_H

#include <stdbool.h>

#include "mongoose.h"
#include "server/server.h"

/**
 * Handle GET /api/reservations and GET /api/reservations/:id.
 * @return true when the request was handled here.
 */
bool reservation_routes_handle(AppContext *app, struct mg_connection *c,
                               const struct mg_http_message *hm);

#endif /* LMS_RESERVATION_ROUTES_H */