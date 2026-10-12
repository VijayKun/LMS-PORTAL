/**
 * report_routes.h - Routes for reports.
 */
#ifndef LMS_REPORT_ROUTES_H
#define LMS_REPORT_ROUTES_H

#include <stdbool.h>

#include "mongoose.h"
#include "server/server.h"

/**
 * Handle GET /api/reports/:type.
 * @return true when the request was handled here.
 */
bool report_routes_handle(AppContext *app, struct mg_connection *c,
                          const struct mg_http_message *hm);

#endif /* LMS_REPORT_ROUTES_H */
