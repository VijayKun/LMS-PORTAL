/**
 * test_student_service.c - Unit tests for the safe-failure behavior of
 * student_service_list(). No live database or network is required: the tests
 * only exercise the "no database" and "bad arguments" branches, which must fail
 * quietly instead of crashing or leaking details.
 *
 * Run with:  make test
 */
#include <stdio.h>
#include <string.h>

#include "services/student_service.h"

static int g_failures = 0;

#define CHECK(cond)                                                       \
  do {                                                                    \
    if (!(cond)) {                                                        \
      printf("  FAIL (line %d): %s\n", __LINE__, #cond);                  \
      g_failures++;                                                       \
    }                                                                     \
  } while (0)

static void test_null_database_is_unavailable(void) {
  char out[128];
  char err[128] = {0};

  StudentListStatus status =
      student_service_list(NULL, out, sizeof(out), err, sizeof(err));
  CHECK(status == STUDENT_LIST_DB_UNAVAILABLE);
}

static void test_disconnected_database_is_unavailable(void) {
  char out[128];
  char err[128] = {0};
  Database db;
  memset(&db, 0, sizeof(db)); /* conn = NULL, connected = false */

  StudentListStatus status =
      student_service_list(&db, out, sizeof(out), err, sizeof(err));
  CHECK(status == STUDENT_LIST_DB_UNAVAILABLE);
}

static void test_null_output_buffer_is_too_small(void) {
  Database db;
  memset(&db, 0, sizeof(db));
  char err[128] = {0};

  StudentListStatus status = student_service_list(&db, NULL, 64, err, sizeof(err));
  CHECK(status == STUDENT_LIST_BUFFER_TOO_SMALL);
}

static void test_zero_size_buffer_is_too_small(void) {
  Database db;
  memset(&db, 0, sizeof(db));
  char out[8] = {0};
  char err[128] = {0};

  StudentListStatus status = student_service_list(&db, out, 0, err, sizeof(err));
  CHECK(status == STUDENT_LIST_BUFFER_TOO_SMALL);
}

static void test_get_empty_id_is_not_found(void) {
  char out[128];
  char err[128] = {0};

  StudentGetStatus status =
      student_service_get(NULL, "", out, sizeof(out), err, sizeof(err));
  CHECK(status == STUDENT_GET_NOT_FOUND);
}

static void test_get_null_database_is_unavailable(void) {
  char out[128];
  char err[128] = {0};

  StudentGetStatus status =
      student_service_get(NULL, "st1", out, sizeof(out), err, sizeof(err));
  CHECK(status == STUDENT_GET_DB_UNAVAILABLE);
}

static void test_get_disconnected_database_is_unavailable(void) {
  Database db;
  memset(&db, 0, sizeof(db)); /* conn = NULL, connected = false */
  char out[128];
  char err[128] = {0};

  StudentGetStatus status =
      student_service_get(&db, "st1", out, sizeof(out), err, sizeof(err));
  CHECK(status == STUDENT_GET_DB_UNAVAILABLE);
}

int main(void) {
  printf("Running student_service tests...\n");
  test_null_database_is_unavailable();
  test_disconnected_database_is_unavailable();
  test_null_output_buffer_is_too_small();
  test_zero_size_buffer_is_too_small();
  test_get_empty_id_is_not_found();
  test_get_null_database_is_unavailable();
  test_get_disconnected_database_is_unavailable();

  if (g_failures == 0) {
    printf("All student_service tests passed.\n");
    return 0;
  }
  printf("%d test(s) failed.\n", g_failures);
  return 1;
}
