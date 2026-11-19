/**
 * notification_controller.h - Request handling for notifications.
 */
#ifndef LMS_NOTIFICATION_CONTROLLER_H
#define LMS_NOTIFICATION_CONTROLLER_H

#include "mongoose.h"
#include "server/server.h"

/** GET /api/notifications - list notifications (filters: user_id, type). */
void notification_controller_list(AppContext *app, struct mg_connection *c,
                                  const struct mg_http_message *hm);

/** GET /api/notifications/:id - return a single notification, or 404 when absent. */
void notification_controller_get(AppContext *app, struct mg_connection *c,
                                 const struct mg_http_message *hm, struct mg_str id);

/** POST /api/notifications - create an announcement (admin only). */
void notification_controller_create(AppContext *app, struct mg_connection *c,
                                    const struct mg_http_message *hm);

/** POST /api/notifications/:id/read - mark one notification read. */
void notification_controller_mark_read(AppContext *app, struct mg_connection *c,
                                       const struct mg_http_message *hm, struct mg_str id);

/** POST /api/notifications/read-all - mark all visible notifications read. */
void notification_controller_mark_all_read(AppContext *app, struct mg_connection *c,
                                           const struct mg_http_message *hm);

#endif /* LMS_NOTIFICATION_CONTROLLER_H */