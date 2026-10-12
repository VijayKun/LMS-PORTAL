/**
 * loan_service.h - Loans (issue/return transactions) business logic boundary.
 */
#ifndef LMS_LOAN_SERVICE_H
#define LMS_LOAN_SERVICE_H

#include <stddef.h>

#include "services/list_service.h"

/** Maximum length of any single editable loan field. */
#define LOAN_FIELD_MAX 128

/** Issue/return policy tuning. Adjust here (per the LMS fair-use policy). */
#define LOAN_DEFAULT_PERIOD_DAYS 14  /* due date = issue date + this */
#define FINE_PER_DAY 10.0            /* rupees per overdue day */

/**
 * Fields accepted when issuing a book. `due_date` is optional: when empty the
 * service computes it as issue date + LOAN_DEFAULT_PERIOD_DAYS.
 */
typedef struct {
  char student_id[LOAN_FIELD_MAX];
  char book_id[LOAN_FIELD_MAX];
  char due_date[LOAN_FIELD_MAX]; /* YYYY-MM-DD or empty */
} LoanIssueInput;

/**
 * Optional filters for a loans list. All fields are optional; a zeroed struct
 * (or NULL) means "first page, no filters".
 */
typedef struct {
  const char *student_id; /* exact match on transactions.student_id */
  const char *book_id;    /* exact match on transactions.book_id */
  const char *status;     /* exact match on transactions.status ('issued'/'returned') */
  int limit;
  int offset;
} LoanListQuery;

/**
 * Load a page of loans from public.transactions, ordered by id.
 *
 * On LIST_OK the output holds {"loans":[...],"limit":L,"offset":O}.
 */
ListStatus loan_service_list(Database *db, const LoanListQuery *query, char *out,
                             size_t out_size, char *errbuf, size_t errlen);

/**
 * Load a single loan by id.
 *
 * On LIST_OK the output holds {"loan":{...}}; missing ids report LIST_NOT_FOUND.
 */
ListStatus loan_service_get(Database *db, const char *id, char *out, size_t out_size,
                            char *errbuf, size_t errlen);

typedef enum {
  LOAN_ISSUE_OK = 0,
  LOAN_ISSUE_BAD_INPUT,          /* missing ids or an invalid due date */
  LOAN_ISSUE_BOOK_NOT_FOUND,     /* the book does not exist */
  LOAN_ISSUE_STUDENT_NOT_FOUND,  /* the student does not exist */
  LOAN_ISSUE_NO_COPY_AVAILABLE,  /* available_copies == 0 (or lost a race) */
  LOAN_ISSUE_DB_UNAVAILABLE,     /* no database configured/connected */
  LOAN_ISSUE_QUERY_FAILED        /* the query itself failed */
} LoanIssueStatus;

/**
 * Issue a book to a student, atomically:
 *   checks the student and book exist, an available copy exists,
 *   decrements books.available_copies and inserts a 'issued' transaction.
 *
 * On LOAN_ISSUE_OK, id_out receives the transaction id.
 */
LoanIssueStatus loan_service_issue(Database *db, const LoanIssueInput *input, char *id_out,
                                   size_t id_size, char *errbuf, size_t errlen);

typedef enum {
  LOAN_RETURN_OK = 0,          /* returned; a fine row may have been created */
  LOAN_RETURN_NOT_FOUND,       /* no 'issued' transaction with this id */
  LOAN_RETURN_BAD_INPUT,       /* empty id */
  LOAN_RETURN_DB_UNAVAILABLE,  /* no database configured/connected */
  LOAN_RETURN_QUERY_FAILED     /* the query itself failed */
} LoanReturnStatus;

/**
 * Return a book, atomically:
 *   marks the transaction 'returned' with today as return date, computes the
 *   overdue fine (0 when on time), inserts a 'pending' fine row when overdue,
 *   and increments books.available_copies. Already-returned transactions are a
 *   no-op (idempotent) reported as LOAN_RETURN_OK.
 */
LoanReturnStatus loan_service_return(Database *db, const char *id, double *fine_out,
                                     int *overdue_days_out, char *errbuf, size_t errlen);

#endif /* LMS_LOAN_SERVICE_H */