/**
 * json_parse.h - Minimal JSON parser for flat request bodies.
 *
 * The API only ever needs string fields out of small flat objects
 * ({"email":"...","password":"..."}). This tiny reader extracts a single string
 * field without pulling in a full JSON library. It is intentionally
 * conservative: unknown/malformed shapes yield PARSE_ERROR, never a crash.
 */
#ifndef LMS_JSON_PARSE_H
#define LMS_JSON_PARSE_H

#include <stddef.h>

typedef enum {
  JSON_PARSE_FOUND = 0,   /* the key was found and copied */
  JSON_PARSE_NOT_FOUND,   /* valid JSON, but the key is absent */
  JSON_PARSE_ERROR        /* malformed JSON or output buffer too small */
} JsonParseResult;

/**
 * Extract the string value of `key` from a flat JSON object.
 *
 * @param json     NUL-terminated JSON text (e.g. a request body)
 * @param key      field name to look up
 * @param out      output buffer for the decoded value
 * @param out_size size of out
 * @return JSON_PARSE_FOUND, JSON_PARSE_NOT_FOUND, or JSON_PARSE_ERROR.
 */
JsonParseResult json_get_string(const char *json, const char *key, char *out,
                                size_t out_size);

#endif /* LMS_JSON_PARSE_H */