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

int main(void) {
  printf("Running isbn_service tests...\n");
  test_missing_output_is_bad_input();
  test_invalid_isbns_are_rejected();

  if (g_failures == 0) {
    printf("All isbn_service tests passed.\n");
    return 0;
  }
  printf("%d test(s) failed.\n", g_failures);
  return 1;
}
