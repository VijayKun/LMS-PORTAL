/**
 * book_controller.h - Request handling for the book catalog.
 */
#ifndef LMS_BOOK_CONTROLLER_H
#define LMS_BOOK_CONTROLLER_H

#include "mongoose.h"
#include "server/server.h"

/** GET /api/books - list books from the database as JSON. */
void book_controller_list(AppContext *app, struct mg_connection *c,
                          const struct mg_http_message *hm);

#endif /* LMS_BOOK_CONTROLLER_H */
