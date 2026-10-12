/**
 * student_controller.h - Request handling for the student directory.
 */
#ifndef LMS_STUDENT_CONTROLLER_H
#define LMS_STUDENT_CONTROLLER_H

#include "mongoose.h"
#include "server/server.h"

/** GET /api/students - list students from the database as JSON. */
void student_controller_list(AppContext *app, struct mg_connection *c,
                             const struct mg_http_message *hm);

/** GET /api/students/:id - return a single student, or 404 when absent. */
void student_controller_get(AppContext *app, struct mg_connection *c,
                            const struct mg_http_message *hm, struct mg_str id);

/** POST /api/students - create a student (a valid session is required). */
void student_controller_create(AppContext *app, struct mg_connection *c,
                               const struct mg_http_message *hm);

/** PUT /api/students/:id - update provided fields (a valid session is required). */
void student_controller_update(AppContext *app, struct mg_connection *c,
                               const struct mg_http_message *hm, struct mg_str id);

/** DELETE /api/students/:id - delete a student (a valid session is required). */
void student_controller_delete(AppContext *app, struct mg_connection *c,
                               const struct mg_http_message *hm, struct mg_str id);

#endif /* LMS_STUDENT_CONTROLLER_H */
