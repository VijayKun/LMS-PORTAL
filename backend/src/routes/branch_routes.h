/**
 * branch_routes.h - Routes for library branches.
 */
#ifndef LMS_BRANCH_ROUTES_H
#define LMS_BRANCH_ROUTES_H

#include <stdbool.h>
#include "mongoose.h"
#include "server/server.h"

bool branch_routes_handle(AppContext *app, struct mg_connection *c,
                          const struct mg_http_message *hm);

#endif
