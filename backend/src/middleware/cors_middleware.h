/**
 * cors_middleware.h - Cross-Origin Resource Sharing headers.
 *
 * The frontend runs on a different origin (Vercel / localhost:5173) than the
 * backend, so the browser needs CORS headers on every response.
 */
#ifndef LMS_CORS_MIDDLEWARE_H
#define LMS_CORS_MIDDLEWARE_H

#include <stdbool.h>
#include <stddef.h>

#include "mongoose.h"
#include "config/config.h"

/** Remember the allowed origin from configuration. Call once at startup. */
void cors_init(const ServerConfig *cfg);

/**
 * Write the CORS response headers into buf.
 * The string is ready to pass to mg_http_reply() (ends with CRLF).
 */
void cors_headers(char *buf, size_t buf_size);

/**
 * Answer a preflight (OPTIONS) request.
 * @return true if the request was an OPTIONS preflight and has been answered.
 */
bool cors_handle_preflight(struct mg_connection *c, const struct mg_http_message *hm);

/**
 * Check a state-changing request's Origin against the configured origin.
 * Requests without an Origin header (curl, native clients) are allowed; browser
 * writes from any other origin are rejected.
 */
bool cors_write_allowed(const struct mg_http_message *hm);

#endif /* LMS_CORS_MIDDLEWARE_H */
