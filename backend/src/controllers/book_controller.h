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

/** GET /api/books/:id - return a single book, or 404 when it does not exist. */
void book_controller_get(AppContext *app, struct mg_connection *c,
                         const struct mg_http_message *hm, struct mg_str id);

/** GET /api/books/isbn/:isbn - return a single book by ISBN, or 404. */
void book_controller_get_by_isbn(AppContext *app, struct mg_connection *c,
                                 const struct mg_http_message *hm, struct mg_str isbn);

/** GET /api/books/lookup/:isbn - Google Books metadata for an ISBN. */
void book_controller_lookup_isbn(AppContext *app, struct mg_connection *c,
                                 const struct mg_http_message *hm, struct mg_str isbn);

/** POST /api/books - create a book (a valid session is required). */
void book_controller_create(AppContext *app, struct mg_connection *c,
                            const struct mg_http_message *hm);

/** PUT /api/books/:id - update provided fields (a valid session is required). */
void book_controller_update(AppContext *app, struct mg_connection *c,
                            const struct mg_http_message *hm, struct mg_str id);

/** DELETE /api/books/:id - delete a book (a valid session is required). */
void book_controller_delete(AppContext *app, struct mg_connection *c,
                            const struct mg_http_message *hm, struct mg_str id);

/** POST /api/books/import - bulk create books from a JSON array (admin session required). */
void book_controller_import(AppContext *app, struct mg_connection *c,
                            const struct mg_http_message *hm);

#endif /* LMS_BOOK_CONTROLLER_H */
