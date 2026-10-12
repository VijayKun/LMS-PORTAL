/**
 * health_routes.h - GET /api/health
 */
#ifndef LMS_HEALTH_ROUTES_H
#define LMS_HEALTH_ROUTES_H

#include <stdbool.h>

#include "mongoose.h"
#include "server/server.h"

/** Record the process start time so uptime can be reported. */
void health_routes_init(void);

/**
 * Handle the request if it is GET /api/health.
 * @return true when the request was handled here.
 */
bool health_routes_handle(AppContext *app, struct mg_connection *c,
                          const struct mg_http_message *hm);

#endif /* LMS_HEALTH_ROUTES_H */
