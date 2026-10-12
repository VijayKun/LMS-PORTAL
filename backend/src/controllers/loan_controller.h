/**
 * loan_controller.h - Request handling for loans.
 */
#ifndef LMS_LOAN_CONTROLLER_H
#define LMS_LOAN_CONTROLLER_H

#include "mongoose.h"
#include "server/server.h"

/** GET /api/transactions - list loans (filters: student_id, book_id, status). */
void loan_controller_list(AppContext *app, struct mg_connection *c,
                          const struct mg_http_message *hm);

/** GET /api/transactions/:id - return a single loan, or 404 when absent. */
void loan_controller_get(AppContext *app, struct mg_connection *c,
                         const struct mg_http_message *hm, struct mg_str id);

/**
 * POST /api/transactions - issue a book to a student.
 * Body: {"student_id","book_id","due_date"?}. Replies 201 with {"loan":{...}}.
 */
void loan_controller_issue(AppContext *app, struct mg_connection *c,
                           const struct mg_http_message *hm);

/**
 * POST /api/transactions/:id/return - return a book, computing any overdue fine.
 * Replies 200 with the updated {"loan":{...}}.
 */
void loan_controller_return(AppContext *app, struct mg_connection *c,
                            const struct mg_http_message *hm, struct mg_str id);

#endif /* LMS_LOAN_CONTROLLER_H */