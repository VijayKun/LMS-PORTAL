/**
 * student_routes.h - Routes for the student directory.
 */
#ifndef LMS_STUDENT_ROUTES_H
#define LMS_STUDENT_ROUTES_H

#include <stdbool.h>

#include "mongoose.h"
#include "server/server.h"

/**
 * Handle GET /api/students.
 * @return true when the request was handled here.
 */
bool student_routes_handle(AppContext *app, struct mg_connection *c,
                           const struct mg_http_message *hm);

#endif /* LMS_STUDENT_ROUTES_H */
