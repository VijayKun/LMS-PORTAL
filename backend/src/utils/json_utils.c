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

/* Write a single JSON object from one row of ncols cells. */
static void w_object(Writer *w, const char *const *keys, const int *kinds,
                     const char *const *values, int ncols) {
  w_putc(w, '{');
  for (int col = 0; col < ncols; col++) {
    if (col > 0) {
      w_putc(w, ',');
    }

    w_string(w, (keys[col] != NULL) ? keys[col] : "");
    w_putc(w, ':');

    const char *value = values[col];
    int kind = (kinds != NULL) ? kinds[col] : JSON_VALUE_STRING;

    if (value == NULL) {
      w_puts(w, "null");
    } else if (kind == JSON_VALUE_NUMBER && is_json_number(value)) {
      w_puts(w, value);
    } else if (kind == JSON_VALUE_BOOL && value[0] != '\0' && value[1] == '\0' &&
               (value[0] == 't' || value[0] == 'f')) {
      w_puts(w, (value[0] == 't') ? "true" : "false");
    } else {
      w_string(w, value);
    }
  }
  w_putc(w, '}');
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
    w_object(&w, keys, kinds, values + (size_t)row * (size_t)ncols, ncols);
  }
  w_putc(&w, ']');

  if (w.overflow) {
    out[out_size - 1] = '\0';
    return -1;
  }

  out[w.len] = '\0';
  return (int)w.len;
}

int json_object_of_row(const char *const *keys, const int *kinds,
                       const char *const *values, int ncols, char *out,
                       size_t out_size) {
  if (out == NULL || out_size == 0 || ncols < 0) {
    return -1;
  }
  if (ncols > 0 && (keys == NULL || values == NULL)) {
    return -1;
  }

  Writer w = {out, out_size, 0, false};
  w_object(&w, keys, kinds, values, ncols);

  if (w.overflow) {
    out[out_size - 1] = '\0';
    return -1;
  }

  out[w.len] = '\0';
  return (int)w.len;
}

int json_array_count(const char *json, const char *key) {
  if (json == NULL) return 0;
  size_t len = strlen(json);
  for (size_t i = 0; i + strlen(key) < len; i++) {
    if (strncmp(json + i, "\"" , 1) == 0 &&
        strncmp(json + i + 1, key, strlen(key)) == 0 &&
        strncmp(json + i + 1 + strlen(key), "\":", 2) == 0) {
      size_t arr_start = i + 1 + strlen(key) + 2;
      while (arr_start < len && (json[arr_start] == ' ' || json[arr_start] == '\n' ||
             json[arr_start] == '\r' || json[arr_start] == '\t')) arr_start++;
      if (json[arr_start] != '[') continue;
      size_t pos = arr_start + 1;
      int depth = 1;
      while (pos < len && depth > 0) {
        if (json[pos] == '[' || json[pos] == '{') depth++;
        else if (json[pos] == ']' || json[pos] == '}') depth--;
        else if (json[pos] == '"' || json[pos] == '\'') {
          char quote = json[pos];
          pos++;
          while (pos < len && json[pos] != quote) {
            if (json[pos] == '\\') pos++;
            pos++;
          }
        }
        pos++;
      }
      if (depth != 0) continue;
      int count = 0;
      bool in_string = false;
      for (size_t j = arr_start + 1; j < pos - 1; j++) {
        char c = json[j];
        if (c == '"' && (j == 0 || json[j-1] != '\\')) in_string = !in_string;
        if (!in_string && c == ',') count++;
      }
      return count + 1;
    }
  }
  return 0;
}

int json_array_get(const char *json, const char *key, int index, char *out, size_t out_size) {
  if (json == NULL || key == NULL || out == NULL || out_size == 0) return -1;
  if (index < 0) return -1;

  size_t len = strlen(json);
  for (size_t i = 0; i + strlen(key) < len; i++) {
    if (strncmp(json + i, "\"" , 1) == 0 &&
        strncmp(json + i + 1, key, strlen(key)) == 0 &&
        strncmp(json + i + 1 + strlen(key), "\":", 2) == 0) {
      size_t arr_start = i + 1 + strlen(key) + 2;
      while (arr_start < len && (json[arr_start] == ' ' || json[arr_start] == '\n' ||
             json[arr_start] == '\r' || json[arr_start] == '\t')) arr_start++;
      if (json[arr_start] != '[') continue;

      size_t pos = arr_start + 1;
      int depth = 1;
      char obj_buf[8192];
      int obj_len = 0;
      bool in_string = false;

      while (pos < len && depth > 0) {
        char c = json[pos];
        if (c == '"' && (pos == 0 || json[pos-1] != '\\')) in_string = !in_string;
        if (!in_string) {
          if (c == '{') {
            if (depth == 1) { obj_len = 0; }
            if (depth == 1 && obj_len < (int)sizeof(obj_buf) - 1) {
              obj_buf[obj_len++] = c;
            }
            depth++;
          } else if (c == '}') {
            depth--;
            if (depth == 1) {
              obj_buf[obj_len] = '\0';
              if (index == 0) {
                strncpy(out, obj_buf, out_size - 1);
                out[out_size - 1] = '\0';
                return obj_len;
              }
              index--;
            }
          } else if (depth == 1 && obj_len < (int)sizeof(obj_buf) - 1) {
            obj_buf[obj_len++] = c;
          }
        } else {
          if (obj_len < (int)sizeof(obj_buf) - 1) {
            obj_buf[obj_len++] = c;
          }
        }
        pos++;
      }
      return -1;
    }
  }
  return -1;
}

bool json_string_of_key(const char *json, const char *key, char *out, size_t out_size) {
  if (json == NULL || key == NULL || out == NULL || out_size == 0) return false;
  size_t len = strlen(json);
  for (size_t i = 0; i + strlen(key) < len; i++) {
    if (json[i] == '"' && strncmp(json + i + 1, key, strlen(key)) == 0 &&
        json[i + 1 + strlen(key)] == '"' && json[i + 2 + strlen(key)] == ':') {
      size_t val_start = i + 2 + strlen(key);
      while (val_start < len && (json[val_start] == ' ' || json[val_start] == '\n' ||
             json[val_start] == '\r' || json[val_start] == '\t')) val_start++;
      if (json[val_start] != '"') return false;
      val_start++;
      size_t val_end = val_start;
      while (val_end < len && json[val_end] != '"') {
        if (json[val_end] == '\\' && val_end + 1 < len) val_end++;
        val_end++;
      }
      size_t copy_len = val_end - val_start;
      if (copy_len >= out_size) return false;
      for (size_t j = 0; j < copy_len; j++) {
        out[j] = json[val_start + j];
        if (json[val_start + j] == '\\' && j + 1 < copy_len) {
          switch (json[val_start + j + 1]) {
            case 'n': out[j] = '\n'; break;
            case 'r': out[j] = '\r'; break;
            case 't': out[j] = '\t'; break;
            case '"': out[j] = '"'; break;
            case '\\': out[j] = '\\'; break;
            default: break;
          }
          if (j + 1 < copy_len) j++;
        }
      }
      out[copy_len] = '\0';
      return true;
    }
  }
  return false;
}
