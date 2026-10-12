/**
 * book_routes.h - Routes for the book catalog.
 */
#ifndef LMS_BOOK_ROUTES_H
#define LMS_BOOK_ROUTES_H

#include <stdbool.h>

#include "mongoose.h"
#include "server/server.h"

/**
 * Handle the request if it is GET /api/books.
 * @return true when the request was handled here.
 */
bool book_routes_handle(AppContext *app, struct mg_connection *c,
                        const struct mg_http_message *hm);

#endif /* LMS_BOOK_ROUTES_H */
