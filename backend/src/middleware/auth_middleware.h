/**
 * auth_middleware.h - Server-side session resolution for protected endpoints.
 *
 * Never trusts a client-supplied role: the role comes from the session, which
 * was created server-side at login.
 */
#ifndef LMS_AUTH_MIDDLEWARE_H
#define LMS_AUTH_MIDDLEWARE_H

#include <stdbool.h>

#include "auth/session_store.h"
#include "mongoose.h"
#include "server/server.h"

/**
 * Resolve the session for this request, or answer 401 when it is missing/
 * expired. Also sweeps expired sessions so the table does not grow unbounded.
 *
 * @param app request context holding the session store
 * @param c   connection (used to send the 401 on failure)
 * @param hm  parsed request
 * @return a live session pointer, or NULL (the caller must stop processing).
 */
Session *auth_require(AppContext *app, struct mg_connection *c,
                      const struct mg_http_message *hm);

/**
 * Extract the bearer token from the request: either the `lms_session` cookie or
 * an `Authorization: Bearer <token>` header (header wins).
 *
 * @param hm       parsed request
 * @param out      output buffer (SESSION_TOKEN_STR bytes recommended)
 * @param out_size size of out
 * @return true and a filled out on success.
 */
bool auth_extract_token(const struct mg_http_message *hm, char *out, size_t out_size);

#endif /* LMS_AUTH_MIDDLEWARE_H */