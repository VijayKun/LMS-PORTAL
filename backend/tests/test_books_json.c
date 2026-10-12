/**
 * test_books_json.c - Unit tests for json_array_of_objects()
 * (the function that turns database rows into the "books" JSON array).
 *
 * Pure string tests: no database or network required.
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

static void test_empty_result(void) {
  char out[32];
  int n = json_array_of_objects(NULL, NULL, NULL, 0, 0, out, sizeof(out));
  CHECK(n == 2);
  CHECK(strcmp(out, "[]") == 0);
}

static void test_string_and_number(void) {
  const char *keys[] = {"name", "copies"};
  int kinds[] = {JSON_VALUE_STRING, JSON_VALUE_NUMBER};
  const char *values[] = {"Clean Code", "3"};
  char out[128];

  int n = json_array_of_objects(keys, kinds, values, 1, 2, out, sizeof(out));
  CHECK(n > 0);
  CHECK(strcmp(out, "[{\"name\":\"Clean Code\",\"copies\":3}]") == 0);
}

static void test_multiple_rows(void) {
  const char *keys[] = {"id"};
  int kinds[] = {JSON_VALUE_NUMBER};
  const char *values[] = {"1", "2"};
  char out[128];

  int n = json_array_of_objects(keys, kinds, values, 2, 1, out, sizeof(out));
  CHECK(n > 0);
  CHECK(strcmp(out, "[{\"id\":1},{\"id\":2}]") == 0);
}

static void test_sql_null_becomes_json_null(void) {
  const char *keys[] = {"id", "author"};
  int kinds[] = {JSON_VALUE_NUMBER, JSON_VALUE_STRING};
  const char *values[] = {NULL, NULL};
  char out[128];

  int n = json_array_of_objects(keys, kinds, values, 1, 2, out, sizeof(out));
  CHECK(n > 0);
  CHECK(strcmp(out, "[{\"id\":null,\"author\":null}]") == 0);
}

static void test_booleans(void) {
  const char *keys[] = {"active"};
  int kinds[] = {JSON_VALUE_BOOL};
  const char *values_t[] = {"t"};
  const char *values_f[] = {"f"};
  char out[64];

  json_array_of_objects(keys, kinds, values_t, 1, 1, out, sizeof(out));
  CHECK(strcmp(out, "[{\"active\":true}]") == 0);

  json_array_of_objects(keys, kinds, values_f, 1, 1, out, sizeof(out));
  CHECK(strcmp(out, "[{\"active\":false}]") == 0);
}

static void test_string_escaping(void) {
  const char *keys[] = {"title"};
  int kinds[] = {JSON_VALUE_STRING};
  const char *values[] = {"He said \"hi\"\\"};
  char out[128];

  int n = json_array_of_objects(keys, kinds, values, 1, 1, out, sizeof(out));
  CHECK(n > 0);
  CHECK(strcmp(out, "[{\"title\":\"He said \\\"hi\\\"\\\\\"}]") == 0);
}

static void test_key_escaping(void) {
  const char *keys[] = {"we\"ird"};
  const char *values[] = {"x"};
  char out[64];

  json_array_of_objects(keys, NULL, values, 1, 1, out, sizeof(out));
  CHECK(strcmp(out, "[{\"we\\\"ird\":\"x\"}]") == 0);
}

static void test_number_fallback_and_signs(void) {
  const char *keys[] = {"n"};
  int kinds[] = {JSON_VALUE_NUMBER};
  char out[64];

  /* Not a valid JSON number -> falls back to a quoted string. */
  const char *bad[] = {"12abc"};
  json_array_of_objects(keys, kinds, bad, 1, 1, out, sizeof(out));
  CHECK(strcmp(out, "[{\"n\":\"12abc\"}]") == 0);

  const char *negative[] = {"-5"};
  json_array_of_objects(keys, kinds, negative, 1, 1, out, sizeof(out));
  CHECK(strcmp(out, "[{\"n\":-5}]") == 0);

  const char *decimal[] = {"3.14"};
  json_array_of_objects(keys, kinds, decimal, 1, 1, out, sizeof(out));
  CHECK(strcmp(out, "[{\"n\":3.14}]") == 0);
}

static void test_null_kinds_defaults_to_string(void) {
  const char *keys[] = {"n"};
  const char *values[] = {"3"};
  char out[64];

  json_array_of_objects(keys, NULL, values, 1, 1, out, sizeof(out));
  CHECK(strcmp(out, "[{\"n\":\"3\"}]") == 0);
}

static void test_overflow_is_safe(void) {
  const char *keys[] = {"name", "copies"};
  int kinds[] = {JSON_VALUE_STRING, JSON_VALUE_NUMBER};
  const char *values[] = {"Clean Code", "3"};
  char out[4];

  int n = json_array_of_objects(keys, kinds, values, 1, 2, out, sizeof(out));
  CHECK(n == -1);
  CHECK(out[sizeof(out) - 1] == '\0'); /* still NUL-terminated */
}

static void test_bad_arguments(void) {
  char out[32];
  const char *keys[] = {"a"};
  const char *values[] = {"b"};

  CHECK(json_array_of_objects(keys, NULL, values, 1, 1, NULL, sizeof(out)) == -1);
  CHECK(json_array_of_objects(keys, NULL, values, 1, 1, out, 0) == -1);
  CHECK(json_array_of_objects(NULL, NULL, values, 1, 1, out, sizeof(out)) == -1);
  CHECK(json_array_of_objects(keys, NULL, NULL, 1, 1, out, sizeof(out)) == -1);
}

int main(void) {
  printf("Running books JSON tests...\n");
  test_empty_result();
  test_string_and_number();
  test_multiple_rows();
  test_sql_null_becomes_json_null();
  test_booleans();
  test_string_escaping();
  test_key_escaping();
  test_number_fallback_and_signs();
  test_null_kinds_defaults_to_string();
  test_overflow_is_safe();
  test_bad_arguments();

  if (g_failures == 0) {
    printf("All books JSON tests passed.\n");
    return 0;
  }
  printf("%d test(s) failed.\n", g_failures);
  return 1;
}
