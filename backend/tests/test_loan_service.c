/**
 * test_loan_service.c - Unit tests for the safe-failure behavior of the loan
 * issue/return write paths. No live database is required: only the "no
 * database" and "bad arguments" branches are exercised.
 *
 * Run with:  make test
 */
#include <stdio.h>
#include <string.h>

#include "services/loan_service.h"

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
  memset(&db, 0, sizeof(db)); /* conn = NULL, connected = false */
  return db;
}

/* ---- issue ---- */

static void test_issue_null_database_is_unavailable(void) {
  char id[64];
  char err[128] = {0};
  LoanIssueInput in;
  memset(&in, 0, sizeof(in));
  strcpy(in.student_id, "st1");
  strcpy(in.book_id, "bk1");

  CHECK(loan_service_issue(NULL, &in, id, sizeof(id), err, sizeof(err)) ==
        LOAN_ISSUE_DB_UNAVAILABLE);
}

static void test_issue_disconnected_database_is_unavailable(void) {
  char id[64];
  char err[128] = {0};
  LoanIssueInput in;
  memset(&in, 0, sizeof(in));
  strcpy(in.student_id, "st1");
  strcpy(in.book_id, "bk1");

  Database db = disconnected();
  CHECK(loan_service_issue(&db, &in, id, sizeof(id), err, sizeof(err)) ==
        LOAN_ISSUE_DB_UNAVAILABLE);
}

static void test_issue_missing_student_is_bad_input(void) {
  char id[64];
  char err[128] = {0};
  LoanIssueInput in;
  memset(&in, 0, sizeof(in));
  strcpy(in.book_id, "bk1");

  CHECK(loan_service_issue(NULL, &in, id, sizeof(id), err, sizeof(err)) ==
        LOAN_ISSUE_BAD_INPUT);
}

static void test_issue_missing_book_is_bad_input(void) {
  char id[64];
  char err[128] = {0};
  LoanIssueInput in;
  memset(&in, 0, sizeof(in));
  strcpy(in.student_id, "st1");

  CHECK(loan_service_issue(NULL, &in, id, sizeof(id), err, sizeof(err)) ==
        LOAN_ISSUE_BAD_INPUT);
}

static void test_issue_null_id_out_is_bad_input(void) {
  char err[128] = {0};
  LoanIssueInput in;
  memset(&in, 0, sizeof(in));
  strcpy(in.student_id, "st1");
  strcpy(in.book_id, "bk1");

  CHECK(loan_service_issue(NULL, &in, NULL, 64, err, sizeof(err)) ==
        LOAN_ISSUE_BAD_INPUT);
}

static void test_issue_bad_due_date_is_bad_input(void) {
  char id[64];
  char err[128] = {0};
  LoanIssueInput in;
  memset(&in, 0, sizeof(in));
  strcpy(in.student_id, "st1");
  strcpy(in.book_id, "bk1");
  strcpy(in.due_date, "not-a-date");
  CHECK(loan_service_issue(NULL, &in, id, sizeof(id), err, sizeof(err)) ==
        LOAN_ISSUE_BAD_INPUT);

  strcpy(in.due_date, "2025-02-31"); /* impossible day */
  CHECK(loan_service_issue(NULL, &in, id, sizeof(id), err, sizeof(err)) ==
        LOAN_ISSUE_BAD_INPUT);
}

static void test_issue_valid_date_passes_validation(void) {
  char id[64];
  char err[128] = {0};
  LoanIssueInput in;
  memset(&in, 0, sizeof(in));
  strcpy(in.student_id, "st1");
  strcpy(in.book_id, "bk1");
  strcpy(in.due_date, "2026-01-31"); /* valid, must not be BAD_INPUT */

  /* No database, so it fails later - proving the date itself was accepted. */
  CHECK(loan_service_issue(NULL, &in, id, sizeof(id), err, sizeof(err)) ==
        LOAN_ISSUE_DB_UNAVAILABLE);
}

/* ---- return ---- */

static void test_return_empty_id_is_bad_input(void) {
  char err[128] = {0};
  double fine = 0.0;
  int days = 0;
  CHECK(loan_service_return(NULL, "", &fine, &days, err, sizeof(err)) ==
        LOAN_RETURN_BAD_INPUT);
}

static void test_return_null_database_is_unavailable(void) {
  char err[128] = {0};
  CHECK(loan_service_return(NULL, "tx1", NULL, NULL, err, sizeof(err)) ==
        LOAN_RETURN_DB_UNAVAILABLE);
}

static void test_return_disconnected_database_is_unavailable(void) {
  char err[128] = {0};
  Database db = disconnected();
  CHECK(loan_service_return(&db, "tx1", NULL, NULL, err, sizeof(err)) ==
        LOAN_RETURN_DB_UNAVAILABLE);
}

int main(void) {
  printf("Running loan_service tests...\n");
  test_issue_null_database_is_unavailable();
  test_issue_disconnected_database_is_unavailable();
  test_issue_missing_student_is_bad_input();
  test_issue_missing_book_is_bad_input();
  test_issue_null_id_out_is_bad_input();
  test_issue_bad_due_date_is_bad_input();
  test_issue_valid_date_passes_validation();
  test_return_empty_id_is_bad_input();
  test_return_null_database_is_unavailable();
  test_return_disconnected_database_is_unavailable();

  if (g_failures == 0) {
    printf("All loan_service tests passed.\n");
    return 0;
  }
  printf("%d test(s) failed.\n", g_failures);
  return 1;
}
