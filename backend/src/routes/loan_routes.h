/**
 * loan_routes.h - Routes for loans.
 */
#ifndef LMS_LOAN_ROUTES_H
#define LMS_LOAN_ROUTES_H

#include <stdbool.h>

#include "mongoose.h"
#include "server/server.h"

/**
 * Handle GET /api/loans and GET /api/loans/:id.
 * @return true when the request was handled here.
 */
bool loan_routes_handle(AppContext *app, struct mg_connection *c,
                        const struct mg_http_message *hm);

#endif /* LMS_LOAN_ROUTES_H */