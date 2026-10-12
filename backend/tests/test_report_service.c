/**
 * test_report_service.c - Unit tests for report_service. No live database.
 *
 * Run with:  make test
 */
#include <stdio.h>
#include <string.h>

#include "services/report_service.h"

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

static void test_check_type_overview_ok(void) {
  CHECK(report_service_check_type("overview") == REPORT_OK);
}

static void test_check_type_unknown(void) {
  CHECK(report_service_check_type("unknown") == REPORT_TYPE_UNSUPPORTED);
  CHECK(report_service_check_type("") == REPORT_TYPE_UNSUPPORTED);
  CHECK(report_service_check_type(NULL) == REPORT_TYPE_UNSUPPORTED);
}

static void test_overview_null_database_unavailable(void) {
  char out[256];
  char err[128] = {0};
  CHECK(report_service_overview(NULL, out, sizeof(out), err, sizeof(err)) ==
        REPORT_DB_UNAVAILABLE);
}

static void test_overview_bad_buffers(void) {
  Database db = disconnected();
  char err[128] = {0};
  CHECK(report_service_overview(&db, NULL, 100, err, sizeof(err)) == REPORT_BUFFER_TOO_SMALL);
  CHECK(report_service_overview(&db, err, 0, err, sizeof(err)) == REPORT_BUFFER_TOO_SMALL);
}

int main(void) {
  printf("Running report_service tests...\n");
  test_check_type_overview_ok();
  test_check_type_unknown();
  test_overview_null_database_unavailable();
  test_overview_bad_buffers();

  if (g_failures == 0) {
    printf("All report_service tests passed.\n");
    return 0;
  }
  printf("%d test(s) failed.\n", g_failures);
  return 1;
}
