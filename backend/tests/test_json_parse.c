/**
 * test_json_parse.c - Unit tests for the minimal JSON string-field reader.
 * Run with:  make test
 */
#include <stdio.h>
#include <string.h>

#include "utils/json_parse.h"

static int g_failures = 0;

#define CHECK(cond)                                                       \
  do {                                                                    \
    if (!(cond)) {                                                        \
      printf("  FAIL (line %d): %s\n", __LINE__, #cond);                  \
      g_failures++;                                                       \
    }                                                                     \
  } while (0)

static void test_basic_fields(void) {
  const char *body = "{\"email\":\"a@b.c\",\"password\":\"hunter2\",\"n\":42}";
  char email[64], pw[64];

  CHECK(json_get_string(body, "email", email, sizeof(email)) == JSON_PARSE_FOUND);
  CHECK(strcmp(email, "a@b.c") == 0);
  CHECK(json_get_string(body, "password", pw, sizeof(pw)) == JSON_PARSE_FOUND);
  CHECK(strcmp(pw, "hunter2") == 0);
}

static void test_missing_key_is_not_found(void) {
  const char *body = "{\"email\":\"a@b.c\"}";
  char out[64];
  CHECK(json_get_string(body, "password", out, sizeof(out)) == JSON_PARSE_NOT_FOUND);
}

static void test_empty_object_is_not_found(void) {
  char out[64];
  CHECK(json_get_string("{}", "email", out, sizeof(out)) == JSON_PARSE_NOT_FOUND);
}

static void test_malformed_is_error(void) {
  char out[64];
  CHECK(json_get_string("not json", "email", out, sizeof(out)) == JSON_PARSE_ERROR);
  CHECK(json_get_string("", "email", out, sizeof(out)) == JSON_PARSE_ERROR);
}

static void test_empty_string_value(void) {
  char out[64];
  CHECK(json_get_string("{\"note\":\"\"}", "note", out, sizeof(out)) == JSON_PARSE_FOUND);
  CHECK(strcmp(out, "") == 0);
}

static void test_skips_non_string_values(void) {
  const char *body =
      "{\"count\":123,\"ok\":true,\"list\":[1,2],\"obj\":{\"x\":1},\"email\":\"x@y.z\"}";
  char out[64];
  CHECK(json_get_string(body, "email", out, sizeof(out)) == JSON_PARSE_FOUND);
  CHECK(strcmp(out, "x@y.z") == 0);

  /* A field whose value is not a string must report NOT_FOUND (not FOUND). */
  CHECK(json_get_string(body, "count", out, sizeof(out)) == JSON_PARSE_NOT_FOUND);
}

static void test_escaped_value_decoding(void) {
  char out[64];
  CHECK(json_get_string("{\"p\":\"a\\\"b\"}", "p", out, sizeof(out)) == JSON_PARSE_FOUND);
  CHECK(strcmp(out, "a\"b") == 0);
  CHECK(json_get_string("{\"p\":\"line\\nbreak\"}", "p", out, sizeof(out)) ==
        JSON_PARSE_FOUND);
  CHECK(strcmp(out, "line\nbreak") == 0);
}

static void test_value_too_long_is_error(void) {
  char small[4];
  CHECK(json_get_string("{\"email\":\"this-value-is-way-too-long\"}", "email", small,
                        sizeof(small)) == JSON_PARSE_ERROR);
}

static void test_null_args_are_error(void) {
  char out[64];
  CHECK(json_get_string(NULL, "email", out, sizeof(out)) == JSON_PARSE_ERROR);
  CHECK(json_get_string("{\"a\":1}", NULL, out, sizeof(out)) == JSON_PARSE_ERROR);
  CHECK(json_get_string("{\"a\":1}", "a", NULL, 4) == JSON_PARSE_ERROR);
}

int main(void) {
  printf("Running json_parse tests...\n");
  test_basic_fields();
  test_missing_key_is_not_found();
  test_empty_object_is_not_found();
  test_malformed_is_error();
  test_empty_string_value();
  test_skips_non_string_values();
  test_escaped_value_decoding();
  test_value_too_long_is_error();
  test_null_args_are_error();

  if (g_failures == 0) {
    printf("All json_parse tests passed.\n");
    return 0;
  }
  printf("%d test(s) failed.\n", g_failures);
  return 1;
}