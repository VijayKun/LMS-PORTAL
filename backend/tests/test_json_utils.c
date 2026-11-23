/**
 * test_json_utils.c - Unit tests for src/utils/json_utils.c
 *
 * Plain C with assert-style checks; no external test framework required.
 * Run with:  make test
 */
#include <stdio.h>
#include <string.h>

#include "utils/json_utils.h"

static int g_failures = 0;

#define CHECK(cond)                                                       \
  do {                                                                    \
    if (!(cond)) {                                                        \
      printf("  FAIL (line %d): %s\n", __LINE__, #cond);                  \
      g_failures++;                                                       \
    }                                                                     \
  } while (0)

static void test_plain_string(void) {
  char out[64];
  int n = json_escape("hello world", out, sizeof(out));
  CHECK(n == 11);
  CHECK(strcmp(out, "hello world") == 0);
}

static void test_quotes_and_backslash(void) {
  char out[64];
  int n = json_escape("say \"hi\"\\", out, sizeof(out));
  CHECK(n > 0);
  CHECK(strcmp(out, "say \\\"hi\\\"\\\\") == 0);
}

static void test_control_characters(void) {
  char out[64];
  int n = json_escape("a\nb\tc", out, sizeof(out));
  CHECK(n > 0);
  CHECK(strcmp(out, "a\\nb\\tc") == 0);
}

static void test_other_control_becomes_unicode(void) {
  char out[64];
  int n = json_escape("\x01", out, sizeof(out));
  CHECK(n > 0);
  CHECK(strcmp(out, "\\u0001") == 0);
}

static void test_empty_string(void) {
  char out[16];
  int n = json_escape("", out, sizeof(out));
  CHECK(n == 0);
  CHECK(strcmp(out, "") == 0);
}

static void test_buffer_too_small_is_safe(void) {
  char out[4];
  int n = json_escape("abcdef", out, sizeof(out));
  CHECK(n == -1);
  CHECK(out[0] == 'a'); /* partially written, still NUL-terminated */
  CHECK(out[3] == '\0');
}

static void test_null_arguments(void) {
  char out[8];
  CHECK(json_escape(NULL, out, sizeof(out)) == -1);
  CHECK(json_escape("x", NULL, sizeof(out)) == -1);
  CHECK(json_escape("x", out, 0) == -1);
}

static void test_json_string_of_key_string(void) {
  char out[32] = "stale";
  CHECK(json_string_of_key("{\"title\":\"Clean Code\"}", "title", out, sizeof(out)));
  CHECK(strcmp(out, "Clean Code") == 0);
}

static void test_json_string_of_key_number(void) {
  char out[32] = "stale";
  CHECK(json_string_of_key("{\"totalCopies\":3}", "totalCopies", out, sizeof(out)));
  CHECK(strcmp(out, "3") == 0);
}

static void test_json_string_of_key_missing_clears_output(void) {
  char out[32] = "stale";
  CHECK(!json_string_of_key("{\"title\":\"Clean Code\"}", "description", out, sizeof(out)));
  CHECK(out[0] == '\0');
}

static void test_json_string_of_key_escapes(void) {
  char out[32] = "stale";
  CHECK(json_string_of_key("{\"description\":\"a\\nb\"}", "description", out, sizeof(out)));
  CHECK(strcmp(out, "a\nb") == 0);
}

int main(void) {
  printf("Running json_utils tests...\n");
  test_plain_string();
  test_quotes_and_backslash();
  test_control_characters();
  test_other_control_becomes_unicode();
  test_empty_string();
  test_buffer_too_small_is_safe();
  test_null_arguments();
  test_json_string_of_key_string();
  test_json_string_of_key_number();
  test_json_string_of_key_missing_clears_output();
  test_json_string_of_key_escapes();

  if (g_failures == 0) {
    printf("All json_utils tests passed.\n");
    return 0;
  }
  printf("%d test(s) failed.\n", g_failures);
  return 1;
}
