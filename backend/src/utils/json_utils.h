/**
 * json_utils.h - Small helpers for building/escaping JSON text.
 *
 * This is intentionally tiny. When responses grow we can swap the manual
 * string building for a real JSON library without changing call sites.
 */
#ifndef LMS_JSON_UTILS_H
#define LMS_JSON_UTILS_H

#include <stdbool.h>
#include <stddef.h>

/** How a database text value should be rendered in JSON. */
typedef enum {
  JSON_VALUE_STRING = 0, /* quote + escape (default, always safe) */
  JSON_VALUE_NUMBER = 1, /* emit unquoted when it looks like a JSON number */
  JSON_VALUE_BOOL = 2    /* emit true/false for PostgreSQL 't'/'f' */
} JsonValueKind;

/**
 * Escape a C string so it is safe inside a JSON string literal.
 *
 * Escapes: " \ and control characters (< 0x20).
 *
 * @param src      input string (must not be NULL)
 * @param dst      output buffer
 * @param dst_size total size of dst, including the terminating NUL
 * @return number of characters written (excluding the NUL), or -1 on error
 */
int json_escape(const char *src, char *dst, size_t dst_size);

/**
 * Build a JSON array of objects from a row-major value matrix.
 *
 * For every cell: a NULL value becomes JSON null; JSON_VALUE_NUMBER values that
 * parse as JSON numbers are emitted unquoted; JSON_VALUE_BOOL values of "t"/"f"
 * become true/false; everything else is emitted as an escaped string. If a
 * "number"/"bool" value does not match the expected shape it safely falls back
 * to a quoted string, so the output is always valid JSON.
 *
 * @param keys    ncols column names (used as object keys)
 * @param kinds   ncols value hints, or NULL to treat every value as a string
 * @param values  nrows * ncols row-major values (NULL entries => JSON null)
 * @param nrows   number of rows
 * @param ncols   number of columns
 * @param out     output buffer
 * @param out_size size of out (including NUL)
 * @return bytes written (excluding NUL), or -1 if it does not fit / bad args
 */
int json_array_of_objects(const char *const *keys, const int *kinds,
                          const char *const *values, int nrows, int ncols,
                          char *out, size_t out_size);

/**
 * Build a single JSON object from one row of values.
 *
 * Same cell rules as json_array_of_objects(), but the output is one `{...}`
 * object rather than an array.
 *
 * @param keys     ncols column names (used as object keys)
 * @param kinds    ncols value hints, or NULL to treat every value as a string
 * @param values   ncols values (NULL entries => JSON null)
 * @param ncols    number of columns
 * @param out      output buffer
 * @param out_size size of out (including NUL)
 * @return bytes written (excluding NUL), or -1 if it does not fit / bad args
 */
int json_object_of_row(const char *const *keys, const int *kinds,
                       const char *const *values, int ncols, char *out,
                       size_t out_size);

int json_array_count(const char *json, const char *key);

int json_array_get(const char *json, const char *key, int index, char *out,
                   size_t out_size);

bool json_string_of_key(const char *json, const char *key, char *out,
                        size_t out_size);

#endif /* LMS_JSON_UTILS_H */
