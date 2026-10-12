/**
 * json_parse.c - See json_parse.h.
 *
 * Hand-written scanner: assumes one top-level object. Escapes are decoded for
 * the common cases (\" \\ \n \r \t); \uXXXX passes through verbatim, which is
 * fine for field values like email/password.
 */
#include "utils/json_parse.h"

#include <stdbool.h>
#include <string.h>

static void skip_ws(const char **p) {
  while (**p == ' ' || **p == '\t' || **p == '\n' || **p == '\r') {
    (*p)++;
  }
}

/* Read a JSON string (opening quote already consumed assumptions handled by the
 * caller). Advances *p past the closing quote. Writes the decoded value to out
 * when out != NULL; with a NULL out it validates + advances without storing. */
static bool read_string(const char **p, char *out, size_t cap) {
  size_t n = 0;
  const char *s = *p;

  if (*s != '"') {
    return false;
  }
  s++;

  while (*s != '\0' && *s != '"') {
    char c = *s;
    if (c == '\\') {
      s++;
      if (*s == '\0') {
        return false;
      }
      switch (*s) {
        case '"':  c = '"';  break;
        case '\\': c = '\\'; break;
        case 'n':  c = '\n'; break;
        case 'r':  c = '\r'; break;
        case 't':  c = '\t'; break;
        case 'u':
          if (out != NULL && n + 6 > cap) {
            return false;
          }
          if (out != NULL) {
            out[n++] = '\\';
            out[n++] = 'u';
            if (s[1] != '\0') out[n++] = s[1];
            if (s[2] != '\0') out[n++] = s[2];
            if (s[3] != '\0') out[n++] = s[3];
            if (s[4] != '\0') out[n++] = s[4];
          }
          s += 4;
          continue;
        default:
          /* Unknown escapes keep the backslash and pass the char through. */
          if (out != NULL && n + 1 >= cap) {
            return false;
          }
          if (out != NULL) out[n++] = '\\';
          c = *s;
          break;
      }
      s++;
    } else {
      s++;
    }

    if (out != NULL) {
      if (n + 1 >= cap) {
        return false; /* no room for the terminator */
      }
      out[n++] = c;
    }
  }

  if (*s != '"') {
    return false;
  }
  *p = s + 1;

  if (out != NULL) {
    out[n] = '\0';
  }
  return true;
}

/* Skip a non-string value (number, true/false/null, nested object/array). */
static bool skip_value(const char **p) {
  const char *s = *p;
  if (s == NULL) {
    return false;
  }

  if (*s == '"') {
    return read_string(&s, NULL, 0);
  }

  if (*s == '{' || *s == '[') {
    char open = *s;
    char close = (open == '{') ? '}' : ']';
    int depth = 0;
    while (*s != '\0') {
      if (*s == '"') {
        if (!read_string(&s, NULL, 0)) {
          return false;
        }
        continue;
      }
      if (*s == open) {
        depth++;
      } else if (*s == close) {
        depth--;
        if (depth == 0) {
          s++;
          *p = s;
          return true;
        }
      }
      s++;
    }
    return false;
  }

  /* Scalar token: [, } or whitespace ends it. */
  while (*s != '\0' && *s != ',' && *s != '}' && *s != ']' && *s != ' ' && *s != '\t' &&
         *s != '\n' && *s != '\r') {
    s++;
  }
  *p = s;
  return true;
}

JsonParseResult json_get_string(const char *json, const char *key, char *out,
                                size_t out_size) {
  if (json == NULL || key == NULL) {
    return JSON_PARSE_ERROR;
  }
  if (out == NULL || out_size == 0) {
    return JSON_PARSE_ERROR;
  }
  out[0] = '\0';

  const char *p = json;
  skip_ws(&p);
  if (*p != '{') {
    return JSON_PARSE_ERROR;
  }
  p++;

  for (;;) {
    skip_ws(&p);
    if (*p == '}') {
      return JSON_PARSE_NOT_FOUND;
    }
    if (*p != '"') {
      return JSON_PARSE_ERROR;
    }

    /* Read the field name into a scratch buffer. */
    char name[128];
    if (!read_string(&p, name, sizeof(name))) {
      return JSON_PARSE_ERROR;
    }

    skip_ws(&p);
    if (*p != ':') {
      return JSON_PARSE_ERROR;
    }
    p++;
    skip_ws(&p);

    if (*p == '"') {
      if (strcmp(name, key) == 0) {
        if (!read_string(&p, out, out_size)) {
          return JSON_PARSE_ERROR; /* value too long for out */
        }
        return JSON_PARSE_FOUND;
      }
      if (!read_string(&p, NULL, 0)) {
        return JSON_PARSE_ERROR;
      }
    } else {
      if (!skip_value(&p)) {
        return JSON_PARSE_ERROR;
      }
    }

    skip_ws(&p);
    if (*p == ',') {
      p++;
      continue;
    }
    if (*p == '}') {
      return JSON_PARSE_NOT_FOUND;
    }
    return JSON_PARSE_ERROR;
  }
}