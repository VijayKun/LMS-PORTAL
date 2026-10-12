/**
 * test_reservation_service.c - Unit tests for the safe-failure behavior of the
 * reservation write paths. No live database is required.
 *
 * Run with:  make test
 */
#include <stdio.h>
#include <string.h>

#include "services/reservation_service.h"

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

/* ---- create ---- */

static void test_create_null_database_is_unavailable(void) {
  char id[64];
  char err[128] = {0};
  ReservationInput in;
  memset(&in, 0, sizeof(in));
  strcpy(in.student_id, "st1");
  strcpy(in.book_id, "bk1");

  CHECK(reservation_service_create(NULL, &in, id, sizeof(id), err, sizeof(err)) ==
        RESERVATION_CREATE_DB_UNAVAILABLE);
}

static void test_create_disconnected_database_is_unavailable(void) {
  char id[64];
  char err[128] = {0};
  ReservationInput in;
  memset(&in, 0, sizeof(in));
  strcpy(in.student_id, "st1");
  strcpy(in.book_id, "bk1");

  Database db = disconnected();
  CHECK(reservation_service_create(&db, &in, id, sizeof(id), err, sizeof(err)) ==
        RESERVATION_CREATE_DB_UNAVAILABLE);
}

static void test_create_missing_student_is_bad_input(void) {
  char id[64];
  char err[128] = {0};
  ReservationInput in;
  memset(&in, 0, sizeof(in));
  strcpy(in.book_id, "bk1");

  CHECK(reservation_service_create(NULL, &in, id, sizeof(id), err, sizeof(err)) ==
        RESERVATION_CREATE_BAD_INPUT);
}

static void test_create_missing_book_is_bad_input(void) {
  char id[64];
  char err[128] = {0};
  ReservationInput in;
  memset(&in, 0, sizeof(in));
  strcpy(in.student_id, "st1");

  CHECK(reservation_service_create(NULL, &in, id, sizeof(id), err, sizeof(err)) ==
        RESERVATION_CREATE_BAD_INPUT);
}

static void test_create_null_id_out_is_bad_input(void) {
  char err[128] = {0};
  ReservationInput in;
  memset(&in, 0, sizeof(in));
  strcpy(in.student_id, "st1");
  strcpy(in.book_id, "bk1");

  CHECK(reservation_service_create(NULL, &in, NULL, 64, err, sizeof(err)) ==
        RESERVATION_CREATE_BAD_INPUT);
}

/* ---- cancel ---- */

static void test_cancel_empty_id_is_bad_input(void) {
  char err[128] = {0};
  CHECK(reservation_service_cancel(NULL, "", err, sizeof(err)) ==
        RESERVATION_CANCEL_BAD_INPUT);
}

static void test_cancel_null_database_is_unavailable(void) {
  char err[128] = {0};
  CHECK(reservation_service_cancel(NULL, "rs1", err, sizeof(err)) ==
        RESERVATION_CANCEL_DB_UNAVAILABLE);
}

/* ---- ready ---- */

static void test_ready_empty_id_is_bad_input(void) {
  char err[128] = {0};
  CHECK(reservation_service_ready(NULL, "", err, sizeof(err)) ==
        RESERVATION_READY_BAD_INPUT);
}

static void test_ready_null_database_is_unavailable(void) {
  char err[128] = {0};
  CHECK(reservation_service_ready(NULL, "rs1", err, sizeof(err)) ==
        RESERVATION_READY_DB_UNAVAILABLE);
}

int main(void) {
  printf("Running reservation_service tests...\n");
  test_create_null_database_is_unavailable();
  test_create_disconnected_database_is_unavailable();
  test_create_missing_student_is_bad_input();
  test_create_missing_book_is_bad_input();
  test_create_null_id_out_is_bad_input();
  test_cancel_empty_id_is_bad_input();
  test_cancel_null_database_is_unavailable();
  test_ready_empty_id_is_bad_input();
  test_ready_null_database_is_unavailable();

  if (g_failures == 0) {
    printf("All reservation_service tests passed.\n");
    return 0;
  }
  printf("%d test(s) failed.\n", g_failures);
  return 1;
}
