/**
 * test_fine_service.c - Unit tests for the safe-failure behavior of fine pay.
 * No live database is required.
 *
 * Run with:  make test
 */
#include <stdio.h>
#include <string.h>

#include "services/fine_service.h"

static int g_failures = 0;

#define CHECK(cond)                                          \
  do {                                                       \
    if (!(cond)) {                                           \
      printf("  FAIL (line %d): %s\n", __LINE__, #cond);     \
      g_failures++;                                          \
    }                                                        \
  } while (0)

static Database disconnected(void) {
  Database db;
  memset(&db, 0, sizeof(db));
  return db;
}

static void test_pay_empty_id_is_bad_input(void) {
  char err[128] = {0};
  CHECK(fine_service_pay(NULL, "", err, sizeof(err)) == FINE_PAY_BAD_INPUT);
}

static void test_pay_null_database_is_unavailable(void) {
  char err[128] = {0};
  CHECK(fine_service_pay(NULL, "fn1", err, sizeof(err)) == FINE_PAY_DB_UNAVAILABLE);
}

static void test_pay_disconnected_database_is_unavailable(void) {
  char err[128] = {0};
  Database db = disconnected();
  CHECK(fine_service_pay(&db, "fn1", err, sizeof(err)) == FINE_PAY_DB_UNAVAILABLE);
}

int main(void) {
  printf("Running fine_service tests...\n");
  test_pay_empty_id_is_bad_input();
  test_pay_null_database_is_unavailable();
  test_pay_disconnected_database_is_unavailable();

  if (g_failures == 0) {
    printf("All fine_service tests passed.\n");
    return 0;
  }
  printf("%d test(s) failed.\n", g_failures);
  return 1;
}
