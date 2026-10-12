/**
 * notification_routes.h - Routes for notifications.
 */
#ifndef LMS_NOTIFICATION_ROUTES_H
#define LMS_NOTIFICATION_ROUTES_H

#include <stdbool.h>

#include "mongoose.h"
#include "server/server.h"

/**
 * Handle GET /api/notifications and GET /api/notifications/:id.
 * @return true when the request was handled here.
 */
bool notification_routes_handle(AppContext *app, struct mg_connection *c,
                                const struct mg_http_message *hm);

#endif /* LMS_NOTIFICATION_ROUTES_H */