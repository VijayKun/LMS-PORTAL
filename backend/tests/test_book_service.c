/**
 * test_book_service.c - Unit tests for the safe-failure behavior of
 * book_service_list(). No live database or network is required: the tests only
 * exercise the "no database" and "bad arguments" branches, which must fail
 * quietly instead of crashing or leaking details.
 *
 * Run with:  make test
 */
#include <stdio.h>
#include <string.h>

#include "services/book_service.h"

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

  BookListStatus status =
      book_service_list(NULL, out, sizeof(out), err, sizeof(err));
  CHECK(status == BOOK_LIST_DB_UNAVAILABLE);
}

static void test_disconnected_database_is_unavailable(void) {
  char out[128];
  char err[128] = {0};
  Database db;
  memset(&db, 0, sizeof(db)); /* conn = NULL, connected = false */

  BookListStatus status =
      book_service_list(&db, out, sizeof(out), err, sizeof(err));
  CHECK(status == BOOK_LIST_DB_UNAVAILABLE);
}

static void test_null_output_buffer_is_too_small(void) {
  Database db;
  memset(&db, 0, sizeof(db));
  char err[128] = {0};

  BookListStatus status = book_service_list(&db, NULL, 64, err, sizeof(err));
  CHECK(status == BOOK_LIST_BUFFER_TOO_SMALL);
}

static void test_zero_size_buffer_is_too_small(void) {
  Database db;
  memset(&db, 0, sizeof(db));
  char out[8] = {0};
  char err[128] = {0};

  BookListStatus status = book_service_list(&db, out, 0, err, sizeof(err));
  CHECK(status == BOOK_LIST_BUFFER_TOO_SMALL);
}

static void test_get_empty_id_is_not_found(void) {
  char out[128];
  char err[128] = {0};

  BookGetStatus status = book_service_get(NULL, "", out, sizeof(out), err, sizeof(err));
  CHECK(status == BOOK_GET_NOT_FOUND);
}

static void test_get_null_database_is_unavailable(void) {
  char out[128];
  char err[128] = {0};

  BookGetStatus status = book_service_get(NULL, "bk1", out, sizeof(out), err, sizeof(err));
  CHECK(status == BOOK_GET_DB_UNAVAILABLE);
}

static void test_get_disconnected_database_is_unavailable(void) {
  Database db;
  memset(&db, 0, sizeof(db)); /* conn = NULL, connected = false */
  char out[128];
  char err[128] = {0};

  BookGetStatus status = book_service_get(&db, "bk1", out, sizeof(out), err, sizeof(err));
  CHECK(status == BOOK_GET_DB_UNAVAILABLE);
}

static void test_get_by_isbn_empty_is_not_found(void) {
  char out[128];
  char err[128] = {0};

  BookGetStatus status =
      book_service_get_by_isbn(NULL, "", out, sizeof(out), err, sizeof(err));
  CHECK(status == BOOK_GET_NOT_FOUND);
}

static void test_get_by_isbn_null_database_is_unavailable(void) {
  char out[128];
  char err[128] = {0};

  BookGetStatus status =
      book_service_get_by_isbn(NULL, "9780132350884", out, sizeof(out), err, sizeof(err));
  CHECK(status == BOOK_GET_DB_UNAVAILABLE);
}

static void test_search_null_database_is_unavailable(void) {
  char out[128];
  char err[128] = {0};

  BookListStatus status = book_service_search(NULL, NULL, out, sizeof(out), err, sizeof(err));
  CHECK(status == BOOK_LIST_DB_UNAVAILABLE);
}

static void test_search_null_output_buffer_is_too_small(void) {
  Database db;
  memset(&db, 0, sizeof(db));
  char err[128] = {0};

  BookListStatus status = book_service_search(&db, NULL, NULL, 64, err, sizeof(err));
  CHECK(status == BOOK_LIST_BUFFER_TOO_SMALL);
}

/* ---- create ---- */

static void test_create_null_database_is_unavailable(void) {
  char err[128] = {0};
  char id[64];
  BookMutate b;
  memset(&b, 0, sizeof(b));
  strcpy(b.title, "A Book");
  strcpy(b.isbn, "9780000000001");

  CHECK(book_service_create(NULL, &b, id, sizeof(id), err, sizeof(err)) ==
        BOOK_CREATE_DB_UNAVAILABLE);
  CHECK(book_service_create(&(Database){0}, &b, id, sizeof(id), err, sizeof(err)) ==
        BOOK_CREATE_DB_UNAVAILABLE);
}

static void test_create_missing_title_is_bad_input(void) {
  char err[128] = {0};
  char id[64];
  BookMutate b;
  memset(&b, 0, sizeof(b));
  strcpy(b.isbn, "9780000000002");

  CHECK(book_service_create(NULL, &b, id, sizeof(id), err, sizeof(err)) ==
        BOOK_CREATE_BAD_INPUT);
}

static void test_create_missing_isbn_is_bad_input(void) {
  char err[128] = {0};
  char id[64];
  BookMutate b;
  memset(&b, 0, sizeof(b));
  strcpy(b.title, "A Book");

  CHECK(book_service_create(NULL, &b, id, sizeof(id), err, sizeof(err)) ==
        BOOK_CREATE_BAD_INPUT);
}

static void test_create_bad_numbers_are_bad_input(void) {
  char err[128] = {0};
  char id[64];
  BookMutate b;
  memset(&b, 0, sizeof(b));
  strcpy(b.title, "A Book");
  strcpy(b.isbn, "9780000000003");
  strcpy(b.total_copies, "not-a-number");
  CHECK(book_service_create(NULL, &b, id, sizeof(id), err, sizeof(err)) ==
        BOOK_CREATE_BAD_INPUT);

  memset(&b, 0, sizeof(b));
  strcpy(b.title, "A Book");
  strcpy(b.isbn, "9780000000003");
  strcpy(b.available_copies, "-1");
  CHECK(book_service_create(NULL, &b, id, sizeof(id), err, sizeof(err)) ==
        BOOK_CREATE_BAD_INPUT);

  memset(&b, 0, sizeof(b));
  strcpy(b.title, "A Book");
  strcpy(b.isbn, "9780000000003");
  strcpy(b.available_copies, "3");
  strcpy(b.total_copies, "2");
  CHECK(book_service_create(NULL, &b, id, sizeof(id), err, sizeof(err)) ==
        BOOK_CREATE_BAD_INPUT);

  memset(&b, 0, sizeof(b));
  strcpy(b.title, "A Book");
  strcpy(b.isbn, "9780000000003");
  strcpy(b.year, "99999");
  CHECK(book_service_create(NULL, &b, id, sizeof(id), err, sizeof(err)) ==
        BOOK_CREATE_BAD_INPUT);
}

static void test_create_bad_date_is_bad_input(void) {
  char err[128] = {0};
  char id[64];
  BookMutate b;
  memset(&b, 0, sizeof(b));
  strcpy(b.title, "A Book");
  strcpy(b.isbn, "9780000000004");
  strcpy(b.publication_date, "not-a-date");
  CHECK(book_service_create(NULL, &b, id, sizeof(id), err, sizeof(err)) ==
        BOOK_CREATE_BAD_INPUT);

  memset(&b, 0, sizeof(b));
  strcpy(b.title, "A Book");
  strcpy(b.isbn, "9780000000004");
  strcpy(b.publication_date, "2025-02-31"); /* invalid day, still rejected */
  CHECK(book_service_create(NULL, &b, id, sizeof(id), err, sizeof(err)) ==
        BOOK_CREATE_BAD_INPUT);
}

/* ---- update ---- */

static void test_update_null_database_is_unavailable(void) {
  char err[128] = {0};
  BookMutate b;
  memset(&b, 0, sizeof(b));
  strcpy(b.title, "Renamed");

  CHECK(book_service_update(NULL, "bk9", &b, err, sizeof(err)) ==
        BOOK_UPDATE_DB_UNAVAILABLE);
}

static void test_update_empty_id_is_not_found(void) {
  char err[128] = {0};
  BookMutate b;
  memset(&b, 0, sizeof(b));
  strcpy(b.title, "Renamed");

  CHECK(book_service_update(NULL, "", &b, err, sizeof(err)) == BOOK_UPDATE_NOT_FOUND);
}

static void test_update_no_fields_is_bad_input(void) {
  char err[128] = {0};
  BookMutate b;
  memset(&b, 0, sizeof(b));

  CHECK(book_service_update(NULL, "bk9", &b, err, sizeof(err)) == BOOK_UPDATE_BAD_INPUT);
}

static void test_update_bad_numbers_are_bad_input(void) {
  char err[128] = {0};
  BookMutate b;
  memset(&b, 0, sizeof(b));
  strcpy(b.total_copies, "7");
  strcpy(b.available_copies, "9");
  CHECK(book_service_update(NULL, "bk9", &b, err, sizeof(err)) == BOOK_UPDATE_BAD_INPUT);
}

/* ---- delete ---- */

static void test_delete_null_database_is_unavailable(void) {
  char err[128] = {0};
  CHECK(book_service_delete(NULL, "bk9", err, sizeof(err)) == BOOK_DELETE_DB_UNAVAILABLE);
}

static void test_delete_empty_id_is_not_found(void) {
  char err[128] = {0};
  CHECK(book_service_delete(NULL, "", err, sizeof(err)) == BOOK_DELETE_NOT_FOUND);
}

int main(void) {
  printf("Running book_service tests...\n");
  test_null_database_is_unavailable();
  test_disconnected_database_is_unavailable();
  test_null_output_buffer_is_too_small();
  test_zero_size_buffer_is_too_small();
  test_get_empty_id_is_not_found();
  test_get_null_database_is_unavailable();
  test_get_disconnected_database_is_unavailable();
  test_get_by_isbn_empty_is_not_found();
  test_get_by_isbn_null_database_is_unavailable();
  test_search_null_database_is_unavailable();
  test_search_null_output_buffer_is_too_small();
  test_create_null_database_is_unavailable();
  test_create_missing_title_is_bad_input();
  test_create_missing_isbn_is_bad_input();
  test_create_bad_numbers_are_bad_input();
  test_create_bad_date_is_bad_input();
  test_update_null_database_is_unavailable();
  test_update_empty_id_is_not_found();
  test_update_no_fields_is_bad_input();
  test_update_bad_numbers_are_bad_input();
  test_delete_null_database_is_unavailable();
  test_delete_empty_id_is_not_found();

  if (g_failures == 0) {
    printf("All book_service tests passed.\n");
    return 0;
  }
  printf("%d test(s) failed.\n", g_failures);
  return 1;
}
