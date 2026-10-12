/**
 * loan_service.c - Loans (public.transactions) lists and issue/return writes.
 */
#include "services/loan_service.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "database/db.h"

static const ListSpec LOAN_SPEC = {
    .response_key = "loans",
    .columns = "id, student_id, book_id, issue_date, due_date, return_date, fine, status",
    .table = "public.transactions",
    .order_by = "id",
    .filter_columns = {"student_id", "book_id", "status", NULL},
};

ListStatus loan_service_list(Database *db, const LoanListQuery *query, char *out,
                             size_t out_size, char *errbuf, size_t errlen) {
  ListQuery q = {0};
  if (query != NULL) {
    q.filter_values[0] = query->student_id;
    q.filter_values[1] = query->book_id;
    q.filter_values[2] = query->status;
    q.limit = query->limit;
    q.offset = query->offset;
  }
  return list_service_list(db, &LOAN_SPEC, &q, out, out_size, errbuf, errlen);
}

ListStatus loan_service_get(Database *db, const char *id, char *out, size_t out_size,
                            char *errbuf, size_t errlen) {
  return list_service_get(db, LOAN_SPEC.table, "id", id, LOAN_SPEC.columns,
                          LOAN_SPEC.response_key, out, out_size, errbuf, errlen);
}

/* ---- Date helpers (UTC civil calendrics, no timezone dependencies) ------ */

/* Days since 1970-01-01 for a proleptic Gregorian (y, m, d). Algorithm after
 * Howard Hinnant's days_from_civil. y is expected >= 1 here (validation below
 * already bounds years to [1000, 9999]). */
static int days_from_civil(int y, int m, int d) {
  y -= m <= 2;
  int era = y / 400;
  unsigned yoe = (unsigned)(y - era * 400);
  unsigned doy = (153u * (unsigned)(m + (m > 2 ? -3 : 9)) + 2u) / 5u + (unsigned)d - 1u;
  unsigned doe = yoe * 365u + yoe / 4u - yoe / 100u + doy;
  return era * 146097 + (int)doe - 719468;
}

/* Inverse of days_from_civil: civil_from_days. */
static void civil_from_days(int z, int *year, int *month, int *day) {
  z += 719468;
  int era = (z >= 0 ? z : z - 146096) / 146097;
  unsigned doe = (unsigned)(z - era * 146097);
  unsigned yoe = (doe - doe / 1460u + doe / 36524u - doe / 146096u) / 365u;
  int y = (int)yoe + era * 400;
  unsigned doy = doe - (365u * yoe + yoe / 4u - yoe / 100u);
  int mp = (int)((5u * doy + 2u) / 153u);
  int d = (int)(doy - (153u * (unsigned)mp + 2u) / 5u + 1u);
  int m = mp + (mp < 10 ? 3 : -9);
  *year = y + (m <= 2);
  *month = m;
  *day = d;
}

/* Validate YYYY-MM-DD is a real calendar date; returns 0 when bad. */
static bool valid_calendar_date(int year, int month, int day) {
  static const int dim[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int max_day = dim[month - 1];
  if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) {
    max_day = 29;
  }
  return day >= 1 && day <= max_day;
}

static int parse_date_days(const char *s) {
  if (s == NULL || s[0] == '\0') {
    return -1;
  }
  int year = 0, month = 0, day = 0;
  char extra = '\0';
  if (sscanf(s, "%4d-%2d-%2d%c", &year, &month, &day, &extra) != 3) {
    return -1;
  }
  if (year < 1000 || year > 9999 || month < 1 || month > 12) {
    return -1;
  }
  if (!valid_calendar_date(year, month, day)) {
    return -1;
  }
  return days_from_civil(year, month, day);
}

/* Write the current UTC date as YYYY-MM-DD. */
static void today_utc_date(char *buf, size_t size) {
  time_t now = time(NULL);
  struct tm tm_utc;
  struct tm *p = gmtime(&now);
  if (p != NULL) {
    tm_utc = *p;
  } else {
    tm_utc.tm_year = 70;
    tm_utc.tm_mon = 0;
    tm_utc.tm_mday = 1;
  }
  strftime(buf, size, "%F", &tm_utc);
}

/* Format an absolute day (days since 1970-01-01) as YYYY-MM-DD. */
static void format_day(int days, char *buf, size_t size) {
  int year = 0, month = 0, day = 0;
  civil_from_days(days, &year, &month, &day);
  snprintf(buf, size, "%04d-%02d-%02d", year, month, day);
}

/* ---- Issue -------------------------------------------------------------- */

#define BOOK_AVAILABILITY_SQL \
  "SELECT available_copies FROM public.books WHERE id = $1"

#define STUDENT_EXISTS_SQL "SELECT id FROM public.students WHERE id = $1"

#define DECREMENT_AVAILABLE_SQL                                                \
  "UPDATE public.books SET available_copies = available_copies - 1"            \
  " WHERE id = $1 AND available_copies > 0 RETURNING available_copies"

#define TRANSACTION_INSERT_SQL                                                  \
  "INSERT INTO public.transactions"                                             \
  " (id, student_id, book_id, issue_date, due_date, status)"                    \
  " VALUES (gen_random_uuid()::text, $1, $2, $3, $4, 'issued') RETURNING id"

static void rollback_quiet(Database *db) {
  char rb[128];
  db_execute(db, "ROLLBACK", NULL, 0, rb, sizeof(rb));
}

LoanIssueStatus loan_service_issue(Database *db, const LoanIssueInput *input, char *id_out,
                                   size_t id_size, char *errbuf, size_t errlen) {
  if (id_out == NULL || id_size == 0) {
    return LOAN_ISSUE_BAD_INPUT;
  }
  if (input == NULL || input->student_id[0] == '\0' || input->book_id[0] == '\0') {
    return LOAN_ISSUE_BAD_INPUT;
  }
  if (input->due_date[0] != '\0' && parse_date_days(input->due_date) < 0) {
    return LOAN_ISSUE_BAD_INPUT;
  }
  if (!db_is_configured(db)) {
    return LOAN_ISSUE_DB_UNAVAILABLE;
  }

  const char *params[1];
  DbResult result;

  params[0] = input->book_id;
  if (!db_query(db, BOOK_AVAILABILITY_SQL, params, 1, &result, errbuf, errlen)) {
    return LOAN_ISSUE_QUERY_FAILED;
  }
  long available = 0;
  if (result.nrows == 0) {
    db_result_free(&result);
    return LOAN_ISSUE_BOOK_NOT_FOUND;
  }
  available = result.values[0] != NULL ? atol(result.values[0]) : 0;
  db_result_free(&result);
  if (available <= 0) {
    return LOAN_ISSUE_NO_COPY_AVAILABLE;
  }

  params[0] = input->student_id;
  if (!db_query(db, STUDENT_EXISTS_SQL, params, 1, &result, errbuf, errlen)) {
    return LOAN_ISSUE_QUERY_FAILED;
  }
  if (result.nrows == 0) {
    db_result_free(&result);
    return LOAN_ISSUE_STUDENT_NOT_FOUND;
  }
  db_result_free(&result);

  /* Due date: caller-provided or issue date + the standard loan period. */
  char issue_date[32];
  char due_date[LOAN_FIELD_MAX];
  today_utc_date(issue_date, sizeof(issue_date));
  if (input->due_date[0] != '\0') {
    snprintf(due_date, sizeof(due_date), "%s", input->due_date);
  } else {
    format_day(parse_date_days(issue_date) + LOAN_DEFAULT_PERIOD_DAYS, due_date,
               sizeof(due_date));
  }

  /* Decrement and insert are atomic in a single transaction. The availability
   * re-check (available_copies > 0) also guards against concurrent issues. */
  if (!db_execute(db, "BEGIN", NULL, 0, errbuf, errlen)) {
    return LOAN_ISSUE_QUERY_FAILED;
  }

  params[0] = input->book_id;
  if (!db_query(db, DECREMENT_AVAILABLE_SQL, params, 1, &result, errbuf, errlen)) {
    rollback_quiet(db);
    return LOAN_ISSUE_QUERY_FAILED;
  }
  if (result.nrows == 0) {
    db_result_free(&result);
    rollback_quiet(db);
    return LOAN_ISSUE_NO_COPY_AVAILABLE;
  }
  db_result_free(&result);

  const char *ins[4];
  ins[0] = input->student_id;
  ins[1] = input->book_id;
  ins[2] = issue_date;
  ins[3] = due_date;
  if (!db_query(db, TRANSACTION_INSERT_SQL, ins, 4, &result, errbuf, errlen)) {
    rollback_quiet(db);
    if (db_has_sqlstate(errbuf, DB_SQLSTATE_FOREIGN_KEY_VIOLATION)) {
      /* A student/book deleted between the existence checks and the insert. */
      if (strstr(errbuf, "student") != NULL) {
        return LOAN_ISSUE_STUDENT_NOT_FOUND;
      }
      return LOAN_ISSUE_BOOK_NOT_FOUND;
    }
    return LOAN_ISSUE_QUERY_FAILED;
  }

  LoanIssueStatus status = LOAN_ISSUE_QUERY_FAILED;
  if (result.nrows == 1 && result.values != NULL && result.values[0] != NULL) {
    snprintf(id_out, id_size, "%s", result.values[0]);
    status = LOAN_ISSUE_OK;
  }
  db_result_free(&result);

  if (status != LOAN_ISSUE_OK) {
    rollback_quiet(db);
    return status;
  }

  if (!db_execute(db, "COMMIT", NULL, 0, errbuf, errlen)) {
    rollback_quiet(db);
    return LOAN_ISSUE_QUERY_FAILED;
  }
  return LOAN_ISSUE_OK;
}

/* ---- Return ------------------------------------------------------------- */

#define TRANSACTION_LOOKUP_SQL \
  "SELECT student_id, book_id, due_date, status FROM public.transactions WHERE id = $1"

#define MARK_RETURNED_SQL                                             \
  "UPDATE public.transactions"                                        \
  " SET return_date = $1, status = 'returned', fine = $2"             \
  " WHERE id = $3 AND status = 'issued' RETURNING id"

#define INCREMENT_AVAILABLE_SQL                                        \
  "UPDATE public.books SET available_copies = available_copies + 1"    \
  " WHERE id = $1 RETURNING id"

#define FINE_INSERT_SQL                                                  \
  "INSERT INTO public.fines"                                             \
  " (id, student_id, book_id, due_date, return_date, overdue_days,"     \
  "  amount, status)"                                                    \
  " VALUES (gen_random_uuid()::text, $1, $2, $3, $4, $5, $6, 'pending')" \
  " RETURNING id"

LoanReturnStatus loan_service_return(Database *db, const char *id, double *fine_out,
                                     int *overdue_days_out, char *errbuf, size_t errlen) {
  if (id == NULL || id[0] == '\0') {
    return LOAN_RETURN_BAD_INPUT;
  }
  if (!db_is_configured(db)) {
    return LOAN_RETURN_DB_UNAVAILABLE;
  }

  const char *params[1] = {id};
  DbResult result;

  if (!db_query(db, TRANSACTION_LOOKUP_SQL, params, 1, &result, errbuf, errlen)) {
    return LOAN_RETURN_QUERY_FAILED;
  }
  if (result.nrows == 0) {
    db_result_free(&result);
    return LOAN_RETURN_NOT_FOUND;
  }

  char student[128];
  char book[128];
  char due[16];
  char status[16];
  snprintf(student, sizeof(student), "%s", result.values[0] ? result.values[0] : "");
  snprintf(book, sizeof(book), "%s", result.values[1] ? result.values[1] : "");
  snprintf(due, sizeof(due), "%s", result.values[2] ? result.values[2] : "");
  snprintf(status, sizeof(status), "%s", result.values[3] ? result.values[3] : "");
  db_result_free(&result);

  if (strcmp(status, "returned") == 0) {
    /* Idempotent: double-submitted returns are a no-op. */
    return LOAN_RETURN_OK;
  }

  char today[16];
  today_utc_date(today, sizeof(today));
  int today_days = parse_date_days(today);
  int due_days = parse_date_days(due);
  int overdue_days = (today_days > 0 && due_days > 0 && today_days > due_days)
                         ? today_days - due_days
                         : 0;
  double fine = overdue_days > 0 ? overdue_days * FINE_PER_DAY : 0.0;
  if (fine_out != NULL) {
    *fine_out = fine;
  }
  if (overdue_days_out != NULL) {
    *overdue_days_out = overdue_days;
  }

  if (!db_execute(db, "BEGIN", NULL, 0, errbuf, errlen)) {
    return LOAN_RETURN_QUERY_FAILED;
  }

  char fine_buf[32];
  snprintf(fine_buf, sizeof(fine_buf), "%.2f", fine);
  const char *mark[3];
  mark[0] = today;
  mark[1] = fine_buf;
  mark[2] = id;
  if (!db_query(db, MARK_RETURNED_SQL, mark, 3, &result, errbuf, errlen)) {
    rollback_quiet(db);
    return LOAN_RETURN_QUERY_FAILED;
  }
  if (result.nrows == 0) {
    db_result_free(&result);
    rollback_quiet(db);
    return LOAN_RETURN_NOT_FOUND;
  }
  db_result_free(&result);

  params[0] = book;
  if (!db_query(db, INCREMENT_AVAILABLE_SQL, params, 1, &result, errbuf, errlen)) {
    rollback_quiet(db);
    return LOAN_RETURN_QUERY_FAILED;
  }
  db_result_free(&result);

  if (overdue_days > 0) {
    char overdue_buf[16];
    snprintf(overdue_buf, sizeof(overdue_buf), "%d", overdue_days);
    const char *fine_rec[6];
    fine_rec[0] = student;
    fine_rec[1] = book;
    fine_rec[2] = due;
    fine_rec[3] = today;
    fine_rec[4] = overdue_buf;
    fine_rec[5] = fine_buf;
    if (!db_query(db, FINE_INSERT_SQL, fine_rec, 6, &result, errbuf, errlen)) {
      rollback_quiet(db);
      return LOAN_RETURN_QUERY_FAILED;
    }
    db_result_free(&result);
  }

  if (!db_execute(db, "COMMIT", NULL, 0, errbuf, errlen)) {
    rollback_quiet(db);
    return LOAN_RETURN_QUERY_FAILED;
  }
  return LOAN_RETURN_OK;
}