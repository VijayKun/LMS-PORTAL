/**
 * branch_controller.h - Request handling for library branches.
 */
#ifndef LMS_BRANCH_CONTROLLER_H
#define LMS_BRANCH_CONTROLLER_H

#include "mongoose.h"
#include "server/server.h"

void branch_controller_list(AppContext *app, struct mg_connection *c,
                            const struct mg_http_message *hm);
void branch_controller_create(AppContext *app, struct mg_connection *c,
                              const struct mg_http_message *hm);
void branch_controller_update(AppContext *app, struct mg_connection *c,
                              const struct mg_http_message *hm, struct mg_str id);
void branch_controller_delete(AppContext *app, struct mg_connection *c,
                              const struct mg_http_message *hm, struct mg_str id);

#endif
