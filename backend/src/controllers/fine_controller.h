/**
 * fine_controller.h - Request handling for fines.
 */
#ifndef LMS_FINE_CONTROLLER_H
#define LMS_FINE_CONTROLLER_H

#include "mongoose.h"
#include "server/server.h"

/** GET /api/fines - list fines (filters: student_id, status). */
void fine_controller_list(AppContext *app, struct mg_connection *c,
                          const struct mg_http_message *hm);

/** GET /api/fines/:id - return a single fine, or 404 when absent. */
void fine_controller_get(AppContext *app, struct mg_connection *c,
                         const struct mg_http_message *hm, struct mg_str id);

/**
 * POST /api/fines/:id/pay - mark a pending fine as paid.
 * Replies 200 with the updated {"fine":{...}}.
 */
void fine_controller_pay(AppContext *app, struct mg_connection *c,
                         const struct mg_http_message *hm, struct mg_str id);

#endif /* LMS_FINE_CONTROLLER_H */