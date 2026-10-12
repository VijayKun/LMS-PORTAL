/**
 * response_utils.h - Consistent JSON responses.
 *
 * Every handler replies through these helpers so that CORS headers and the
 * JSON content type are always applied the same way.
 */
#ifndef LMS_RESPONSE_UTILS_H
#define LMS_RESPONSE_UTILS_H

#include "mongoose.h"

/** Send a JSON body with the given HTTP status. */
void send_json(struct mg_connection *c, int status, const char *body);

/** Send a JSON error object: {"error": code, "message": message}. */
void send_json_error(struct mg_connection *c, int status, const char *code,
                     const char *message);

#endif /* LMS_RESPONSE_UTILS_H */
