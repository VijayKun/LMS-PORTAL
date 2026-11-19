/**
 * reservation_controller.h - Request handling for reservations.
 */
#ifndef LMS_RESERVATION_CONTROLLER_H
#define LMS_RESERVATION_CONTROLLER_H

#include "mongoose.h"
#include "server/server.h"

/** GET /api/reservations - list reservations (filters: student_id, book_id, status). */
void reservation_controller_list(AppContext *app, struct mg_connection *c,
                                 const struct mg_http_message *hm);

/** GET /api/reservations/:id - return a single reservation, or 404 when absent. */
void reservation_controller_get(AppContext *app, struct mg_connection *c,
                                const struct mg_http_message *hm, struct mg_str id);

/**
 * POST /api/reservations - place a reservation.
 * Body: {"student_id","book_id"}. Replies 201 with {"reservation":{...}}.
 */
void reservation_controller_create(AppContext *app, struct mg_connection *c,
                                   const struct mg_http_message *hm);

/**
 * DELETE /api/reservations/:id - cancel a waiting/ready reservation.
 * Replies 200 with {"ok":true}.
 */
void reservation_controller_cancel(AppContext *app, struct mg_connection *c,
                                   const struct mg_http_message *hm, struct mg_str id);

/**
 * POST /api/reservations/:id/ready - mark a waiting reservation ready for pickup.
 * Replies 200 with the updated {"reservation":{...}}.
 */
void reservation_controller_ready(AppContext *app, struct mg_connection *c,
                                  const struct mg_http_message *hm, struct mg_str id);

/**
 * POST /api/reservations/:id/status - admin status transition.
 * Body: {"status":"waiting"|"ready"|"fulfilled"|"expired"}.
 * Replies 200 with the updated {"reservation":{...}}.
 */
void reservation_controller_set_status(AppContext *app, struct mg_connection *c,
                                       const struct mg_http_message *hm,
                                       struct mg_str id);

#endif /* LMS_RESERVATION_CONTROLLER_H */