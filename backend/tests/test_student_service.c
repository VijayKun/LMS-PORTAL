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

static void test_search_null_database_is_unavailable(void) {
  char out[128];
  char err[128] = {0};

  StudentListStatus status = student_service_search(NULL, NULL, out, sizeof(out), err,
                                                    sizeof(err));
  CHECK(status == STUDENT_LIST_DB_UNAVAILABLE);
}

static void test_search_disconnected_database_is_unavailable(void) {
  Database db;
  memset(&db, 0, sizeof(db));
  char out[128];
  char err[128] = {0};

  StudentListStatus status = student_service_search(&db, NULL, out, sizeof(out), err,
                                                    sizeof(err));
  CHECK(status == STUDENT_LIST_DB_UNAVAILABLE);
}

static void test_search_null_output_buffer_is_too_small(void) {
  Database db;
  memset(&db, 0, sizeof(db));
  char err[128] = {0};

  StudentListStatus status = student_service_search(&db, NULL, NULL, 64, err, sizeof(err));
  CHECK(status == STUDENT_LIST_BUFFER_TOO_SMALL);
}

/* ---- create ---- */

static StudentMutate make_valid_student(void) {
  StudentMutate s;
  memset(&s, 0, sizeof(s));
  strcpy(s.name, "Ada Lovelace");
  strcpy(s.email, "ada@college.edu");
  strcpy(s.student_id, "22CS001");
  return s;
}

static void test_create_null_database_is_unavailable(void) {
  char err[128] = {0};
  char id[64];
  StudentMutate s = make_valid_student();
  CHECK(student_service_create(NULL, &s, id, sizeof(id), err, sizeof(err)) ==
        STUDENT_CREATE_DB_UNAVAILABLE);
  Database db;
  memset(&db, 0, sizeof(db));
  CHECK(student_service_create(&db, &s, id, sizeof(id), err, sizeof(err)) ==
        STUDENT_CREATE_DB_UNAVAILABLE);
}

static void test_create_missing_required_is_bad_input(void) {
  char err[128] = {0};
  char id[64];

  StudentMutate s = make_valid_student();
  s.name[0] = '\0';
  CHECK(student_service_create(NULL, &s, id, sizeof(id), err, sizeof(err)) ==
        STUDENT_CREATE_BAD_INPUT);

  s = make_valid_student();
  s.email[0] = '\0';
  CHECK(student_service_create(NULL, &s, id, sizeof(id), err, sizeof(err)) ==
        STUDENT_CREATE_BAD_INPUT);

  s = make_valid_student();
  s.student_id[0] = '\0';
  CHECK(student_service_create(NULL, &s, id, sizeof(id), err, sizeof(err)) ==
        STUDENT_CREATE_BAD_INPUT);
}

static void test_create_bad_values_are_bad_input(void) {
  char err[128] = {0};
  char id[64];

  StudentMutate s = make_valid_student();
  strcpy(s.semester, "zero");
  CHECK(student_service_create(NULL, &s, id, sizeof(id), err, sizeof(err)) ==
        STUDENT_CREATE_BAD_INPUT);

  s = make_valid_student();
  strcpy(s.semester, "0");
  CHECK(student_service_create(NULL, &s, id, sizeof(id), err, sizeof(err)) ==
        STUDENT_CREATE_BAD_INPUT);

  s = make_valid_student();
  strcpy(s.status, "banned");
  CHECK(student_service_create(NULL, &s, id, sizeof(id), err, sizeof(err)) ==
        STUDENT_CREATE_BAD_INPUT);

  s = make_valid_student();
  strcpy(s.joined_date, "2024-13-01");
  CHECK(student_service_create(NULL, &s, id, sizeof(id), err, sizeof(err)) ==
        STUDENT_CREATE_BAD_INPUT);
}

static void test_create_valid_statuses_are_accepted(void) {
  char err[128] = {0};
  char id[64];

  StudentMutate s = make_valid_student();
  strcpy(s.status, "active");
  CHECK(student_service_create(NULL, &s, id, sizeof(id), err, sizeof(err)) ==
        STUDENT_CREATE_DB_UNAVAILABLE);

  s = make_valid_student();
  strcpy(s.status, "suspended");
  CHECK(student_service_create(NULL, &s, id, sizeof(id), err, sizeof(err)) ==
        STUDENT_CREATE_DB_UNAVAILABLE);
}

/* ---- update ---- */

static void test_update_null_database_is_unavailable(void) {
  char err[128] = {0};
  StudentMutate s;
  memset(&s, 0, sizeof(s));
  strcpy(s.name, "Renamed");
  CHECK(student_service_update(NULL, "st9", &s, err, sizeof(err)) ==
        STUDENT_UPDATE_DB_UNAVAILABLE);
}

static void test_update_empty_id_is_not_found(void) {
  char err[128] = {0};
  StudentMutate s;
  memset(&s, 0, sizeof(s));
  strcpy(s.name, "Renamed");
  CHECK(student_service_update(NULL, "", &s, err, sizeof(err)) == STUDENT_UPDATE_NOT_FOUND);
}

static void test_update_no_fields_is_bad_input(void) {
  char err[128] = {0};
  StudentMutate s;
  memset(&s, 0, sizeof(s));
  CHECK(student_service_update(NULL, "st9", &s, err, sizeof(err)) == STUDENT_UPDATE_BAD_INPUT);
}

static void test_update_bad_values_are_bad_input(void) {
  char err[128] = {0};
  StudentMutate s;
  memset(&s, 0, sizeof(s));
  strcpy(s.status, "expelled");
  CHECK(student_service_update(NULL, "st9", &s, err, sizeof(err)) == STUDENT_UPDATE_BAD_INPUT);
}

/* ---- delete ---- */

static void test_delete_null_database_is_unavailable(void) {
  char err[128] = {0};
  CHECK(student_service_delete(NULL, "st9", err, sizeof(err)) == STUDENT_DELETE_DB_UNAVAILABLE);
}

static void test_delete_empty_id_is_not_found(void) {
  char err[128] = {0};
  CHECK(student_service_delete(NULL, "", err, sizeof(err)) == STUDENT_DELETE_NOT_FOUND);
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
  test_search_null_database_is_unavailable();
  test_search_disconnected_database_is_unavailable();
  test_search_null_output_buffer_is_too_small();
  test_create_null_database_is_unavailable();
  test_create_missing_required_is_bad_input();
  test_create_bad_values_are_bad_input();
  test_create_valid_statuses_are_accepted();
  test_update_null_database_is_unavailable();
  test_update_empty_id_is_not_found();
  test_update_no_fields_is_bad_input();
  test_update_bad_values_are_bad_input();
  test_delete_null_database_is_unavailable();
  test_delete_empty_id_is_not_found();

  if (g_failures == 0) {
    printf("All student_service tests passed.\n");
    return 0;
  }
  printf("%d test(s) failed.\n", g_failures);
  return 1;
}
