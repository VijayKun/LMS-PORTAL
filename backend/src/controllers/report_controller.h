/**
 * report_controller.h - Request handling for reports.
 */
#ifndef LMS_REPORT_CONTROLLER_H
#define LMS_REPORT_CONTROLLER_H

#include "mongoose.h"
#include "server/server.h"

/**
 * GET /api/reports/:type - build the requested report.
 * Unknown types answer 404; "overview" is the only supported type for now.
 */
void report_controller_get(AppContext *app, struct mg_connection *c,
                           const struct mg_http_message *hm, struct mg_str type);

#endif /* LMS_REPORT_CONTROLLER_H */
