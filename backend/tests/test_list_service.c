/**
 * test_list_service.c - Unit tests for the safe-failure behavior of the shared
 * read/list service engine. No live database or network is required: the tests
 * only exercise the "no database", "bad arguments" and "missing row" branches.
 *
 * Run with:  make test
 */
#include <stdio.h>
#include <string.h>

#include "services/list_service.h"

static int g_failures = 0;

#define CHECK(cond)                                                       \
  do {                                                                    \
    if (!(cond)) {                                                        \
      printf("  FAIL (line %d): %s\n", __LINE__, #cond);                  \
      g_failures++;                                                       \
    }                                                                     \
  } while (0)

static const ListSpec TEST_SPEC = {
    .response_key = "widgets",
    .columns = "id, name",
    .table = "public.widgets",
    .order_by = "id",
    .filter_columns = {"owner", "status", NULL},
};

static void test_list_null_database_is_unavailable(void) {
  char out[128];
  char err[128] = {0};

  ListStatus status = list_service_list(NULL, &TEST_SPEC, NULL, out, sizeof(out), err,
                                        sizeof(err));
  CHECK(status == LIST_DB_UNAVAILABLE);
}

static void test_list_disconnected_database_is_unavailable(void) {
  Database db;
  memset(&db, 0, sizeof(db)); /* conn = NULL, connected = false */
  char out[128];
  char err[128] = {0};

  ListStatus status = list_service_list(&db, &TEST_SPEC, NULL, out, sizeof(out), err,
                                        sizeof(err));
  CHECK(status == LIST_DB_UNAVAILABLE);
}

static void test_list_null_output_buffer_is_too_small(void) {
  Database db;
  memset(&db, 0, sizeof(db));
  char err[128] = {0};

  ListStatus status = list_service_list(&db, &TEST_SPEC, NULL, NULL, 64, err, sizeof(err));
  CHECK(status == LIST_BUFFER_TOO_SMALL);
}

static void test_list_null_spec_is_rejected(void) {
  Database db;
  memset(&db, 0, sizeof(db));
  char out[128];
  char err[128] = {0};

  ListStatus status = list_service_list(&db, NULL, NULL, out, sizeof(out), err, sizeof(err));
  CHECK(status == LIST_BUFFER_TOO_SMALL);
}

static void test_get_empty_pk_is_not_found(void) {
  Database db;
  memset(&db, 0, sizeof(db));
  char out[128];
  char err[128] = {0};

  ListStatus status = list_service_get(&db, "public.widgets", "id", "", "id, name",
                                       "widget", out, sizeof(out), err, sizeof(err));
  CHECK(status == LIST_NOT_FOUND);
}

static void test_get_null_database_is_unavailable(void) {
  char out[128];
  char err[128] = {0};

  ListStatus status = list_service_get(NULL, "public.widgets", "id", "w1", "id, name",
                                       "widget", out, sizeof(out), err, sizeof(err));
  CHECK(status == LIST_DB_UNAVAILABLE);
}

static void test_get_disconnected_database_is_unavailable(void) {
  Database db;
  memset(&db, 0, sizeof(db));
  char out[128];
  char err[128] = {0};

  ListStatus status = list_service_get(&db, "public.widgets", "id", "w1", "id, name",
                                       "widget", out, sizeof(out), err, sizeof(err));
  CHECK(status == LIST_DB_UNAVAILABLE);
}

static void test_get_null_output_buffer_is_too_small(void) {
  Database db;
  memset(&db, 0, sizeof(db));
  char err[128] = {0};

  ListStatus status = list_service_get(&db, "public.widgets", "id", "w1", "id, name",
                                       "widget", NULL, 64, err, sizeof(err));
  CHECK(status == LIST_BUFFER_TOO_SMALL);
}

int main(void) {
  printf("Running list_service tests...\n");
  test_list_null_database_is_unavailable();
  test_list_disconnected_database_is_unavailable();
  test_list_null_output_buffer_is_too_small();
  test_list_null_spec_is_rejected();
  test_get_empty_pk_is_not_found();
  test_get_null_database_is_unavailable();
  test_get_disconnected_database_is_unavailable();
  test_get_null_output_buffer_is_too_small();

  if (g_failures == 0) {
    printf("All list_service tests passed.\n");
    return 0;
  }
  printf("%d test(s) failed.\n", g_failures);
  return 1;
}