/**
 * json_utils.c - See json_utils.h.
 */
#include "utils/json_utils.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

int json_escape(const char *src, char *dst, size_t dst_size) {
  if (src == NULL || dst == NULL || dst_size == 0) {
    return -1;
  }

  size_t out = 0;
  for (const unsigned char *p = (const unsigned char *)src; *p != '\0'; p++) {
    const char *replacement = NULL;
    char unicode[8];

    switch (*p) {
      case '"':  replacement = "\\\""; break;
      case '\\': replacement = "\\\\"; break;
      case '\b': replacement = "\\b";  break;
      case '\f': replacement = "\\f";  break;
      case '\n': replacement = "\\n";  break;
      case '\r': replacement = "\\r";  break;
      case '\t': replacement = "\\t";  break;
      default:
        if (*p < 0x20) {
          /* Other control characters -> \u00XX */
          snprintf(unicode, sizeof(unicode), "\\u%04x", *p);
          replacement = unicode;
        }
        break;
    }

    if (replacement != NULL) {
      size_t len = strlen(replacement);
      if (out + len + 1 > dst_size) {
        dst[out] = '\0';
        return -1; /* would not fit */
      }
      memcpy(dst + out, replacement, len);
      out += len;
    } else {
      if (out + 1 + 1 > dst_size) {
        dst[out] = '\0';
        return -1;
      }
      dst[out++] = (char)*p;
    }
  }

  dst[out] = '\0';
  return (int)out;
}

/* ---- Growing-into-a-fixed-buffer writer -------------------------------- */

typedef struct {
  char *buf;
  size_t cap;
  size_t len;
  bool overflow;
} Writer;

static void w_putc(Writer *w, char ch) {
  if (w->len + 1 >= w->cap) {
    w->overflow = true;
    return;
  }
  w->buf[w->len++] = ch;
}

static void w_puts(Writer *w, const char *s) {
  while (*s != '\0') {
    w_putc(w, *s++);
  }
}

/* Write a JSON string literal (including the surrounding quotes). */
static void w_string(Writer *w, const char *s) {
  w_putc(w, '"');
  for (const unsigned char *p = (const unsigned char *)s; *p != '\0'; p++) {
    switch (*p) {
      case '"':  w_puts(w, "\\\""); break;
      case '\\': w_puts(w, "\\\\"); break;
      case '\b': w_puts(w, "\\b");  break;
      case '\f': w_puts(w, "\\f");  break;
      case '\n': w_puts(w, "\\n");  break;
      case '\r': w_puts(w, "\\r");  break;
      case '\t': w_puts(w, "\\t");  break;
      default:
        if (*p < 0x20) {
          char unicode[8];
          snprintf(unicode, sizeof(unicode), "\\u%04x", *p);
          w_puts(w, unicode);
        } else {
          w_putc(w, (char)*p);
        }
        break;
    }
  }
  w_putc(w, '"');
}

/* Strict enough JSON number check: [-]int[.frac][e[+/-]exp] */
static bool is_json_number(const char *s) {
  if (s == NULL || *s == '\0') {
    return false;
  }

  const char *p = s;
  if (*p == '-') {
    p++;
  }
  if (!isdigit((unsigned char)*p)) {
    return false;
  }
  if (*p == '0') {
    p++;
  } else {
    while (isdigit((unsigned char)*p)) {
      p++;
    }
  }
  if (*p == '.') {
    p++;
    if (!isdigit((unsigned char)*p)) {
      return false;
    }
    while (isdigit((unsigned char)*p)) {
      p++;
    }
  }
  if (*p == 'e' || *p == 'E') {
    p++;
    if (*p == '+' || *p == '-') {
      p++;
    }
    if (!isdigit((unsigned char)*p)) {
      return false;
    }
    while (isdigit((unsigned char)*p)) {
      p++;
    }
  }
  return *p == '\0';
}

int json_array_of_objects(const char *const *keys, const int *kinds,
                          const char *const *values, int nrows, int ncols,
                          char *out, size_t out_size) {
  if (out == NULL || out_size == 0 || nrows < 0 || ncols < 0) {
    return -1;
  }
  if (ncols > 0 && (keys == NULL || (nrows > 0 && values == NULL))) {
    return -1;
  }

  Writer w = {out, out_size, 0, false};

  w_putc(&w, '[');
  for (int row = 0; row < nrows && !w.overflow; row++) {
    if (row > 0) {
      w_putc(&w, ',');
    }
    w_putc(&w, '{');
    for (int col = 0; col < ncols; col++) {
      if (col > 0) {
        w_putc(&w, ',');
      }

      w_string(&w, (keys[col] != NULL) ? keys[col] : "");
      w_putc(&w, ':');

      const char *value = values[row * ncols + col];
      int kind = (kinds != NULL) ? kinds[col] : JSON_VALUE_STRING;

      if (value == NULL) {
        w_puts(&w, "null");
      } else if (kind == JSON_VALUE_NUMBER && is_json_number(value)) {
        w_puts(&w, value);
      } else if (kind == JSON_VALUE_BOOL && value[0] != '\0' && value[1] == '\0' &&
                 (value[0] == 't' || value[0] == 'f')) {
        w_puts(&w, (value[0] == 't') ? "true" : "false");
      } else {
        w_string(&w, value);
      }
    }
    w_putc(&w, '}');
  }
  w_putc(&w, ']');

  if (w.overflow) {
    out[out_size - 1] = '\0';
    return -1;
  }

  out[w.len] = '\0';
  return (int)w.len;
}
