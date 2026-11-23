/**
 * test_isbn_service.c - Input validation for the Google Books lookup.
 * No network access is required: only the rejection paths are exercised.
 *
 * Run with:  make test
 */
#include <stdio.h>
#include <string.h>

#include "services/isbn_service.h"

static int g_failures = 0;

#define CHECK(cond)                                          \
  do {                                                       \
    if (!(cond)) {                                           \
      printf("  FAIL (line %d): %s\n", __LINE__, #cond);     \
      g_failures++;                                          \
    }                                                        \
  } while (0)

static void test_missing_output_is_bad_input(void) {
  char err[128] = {0};
  CHECK(isbn_service_lookup(NULL, "9780132350884", NULL, err, sizeof(err)) ==
        ISBN_LOOKUP_BAD_INPUT);
}

static void test_invalid_isbns_are_rejected(void) {
  IsbnInfo info;
  char err[128] = {0};

  CHECK(isbn_service_lookup(NULL, NULL, &info, err, sizeof(err)) ==
        ISBN_LOOKUP_BAD_INPUT);
  CHECK(isbn_service_lookup(NULL, "", &info, err, sizeof(err)) ==
        ISBN_LOOKUP_BAD_INPUT);
  CHECK(isbn_service_lookup(NULL, "123", &info, err, sizeof(err)) ==
        ISBN_LOOKUP_BAD_INPUT);
  CHECK(isbn_service_lookup(NULL, "978013235088X", &info, err, sizeof(err)) ==
        ISBN_LOOKUP_BAD_INPUT);
  CHECK(isbn_service_lookup(NULL, "abcdefghij", &info, err, sizeof(err)) ==
        ISBN_LOOKUP_BAD_INPUT);
}

static void test_metadata_json_is_complete_and_escaped(void) {
  IsbnInfo info;
  memset(&info, 0, sizeof(info));
  snprintf(info.title, sizeof(info.title), "Clean Code");
  snprintf(info.page_count, sizeof(info.page_count), "464");
  snprintf(info.language, sizeof(info.language), "en");
  snprintf(info.description, sizeof(info.description), "A \"quoted\"\nline");

  char out[ISBN_META_JSON_MAX];
  isbn_service_metadata_json(&info, out, sizeof(out));

  CHECK(out[0] == '{' && out[strlen(out) - 1] == '}');
  CHECK(strstr(out, "\"pageCount\":\"464\"") != NULL);
  CHECK(strstr(out, "\"language\":\"en\"") != NULL);
  CHECK(strstr(out, "\"title\":\"Clean Code\"") != NULL);
  CHECK(strstr(out, "\\\"quoted\\\"") != NULL);
  CHECK(strstr(out, "\\n") != NULL);
}

int main(void) {
  printf("Running isbn_service tests...\n");
  test_missing_output_is_bad_input();
  test_invalid_isbns_are_rejected();
  test_metadata_json_is_complete_and_escaped();

  if (g_failures == 0) {
    printf("All isbn_service tests passed.\n");
    return 0;
  }
  printf("%d test(s) failed.\n", g_failures);
  return 1;
}
