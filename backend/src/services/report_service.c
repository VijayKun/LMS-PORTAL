/**
 * report_service.c - Summary counts and recent activity for the dashboard.
 *
 * The overview report is assembled from a handful of scalar COUNT/SUM queries
 * plus one recent-transactions list. Each scalar query is small and indexed;
 * this is fine for the catalog sizes this LMS targets.
 */
#include "services/report_service.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/json_utils.h"

ReportStatus report_service_check_type(const char *type) {
  if (type == NULL || type[0] == '\0') {
    return REPORT_TYPE_UNSUPPORTED;
  }
  if (strcmp(type, "overview") == 0) {
    return REPORT_OK;
  }
  return REPORT_TYPE_UNSUPPORTED;
}

/* Run a single-value query and return it as a long (0 on no rows/NULL). */
static bool scalar_long(Database *db, const char *sql, long *out, char *errbuf,
                        size_t errlen) {
  DbResult result;
  if (!db_query(db, sql, NULL, 0, &result, errbuf, errlen)) {
    return false;
  }
  long value = 0;
  if (result.nrows > 0 && result.values != NULL && result.values[0] != NULL) {
    value = atol(result.values[0]);
  }
  db_result_free(&result);
  *out = value;
  return true;
}

#define RECENT_TX_SQL                                                         \
  "SELECT id, student_id, book_id, issue_date, due_date, return_date, status" \
  " FROM public.transactions ORDER BY created_at DESC LIMIT 10"

/* Column value hints for the recent-transactions response. */
static const int RECENT_KINDS[7] = {
    JSON_VALUE_STRING, JSON_VALUE_STRING, JSON_VALUE_STRING, JSON_VALUE_STRING,
    JSON_VALUE_STRING, JSON_VALUE_STRING, JSON_VALUE_STRING,
};

ReportStatus report_service_overview(Database *db, char *out, size_t out_size,
                                     char *errbuf, size_t errlen) {
  if (out == NULL || out_size == 0) {
    return REPORT_BUFFER_TOO_SMALL;
  }
  if (!db_is_configured(db)) {
    return REPORT_DB_UNAVAILABLE;
  }

  long total_books = 0, available_copies = 0, total_students = 0;
  long active_loans = 0, overdue_loans = 0, pending_fines = 0, pending_amount = 0;

  if (!scalar_long(db, "SELECT COUNT(*) FROM public.books", &total_books, errbuf, errlen) ||
      !scalar_long(db, "SELECT COALESCE(SUM(available_copies), 0) FROM public.books",
                   &available_copies, errbuf, errlen) ||
      !scalar_long(db, "SELECT COUNT(*) FROM public.students", &total_students, errbuf,
                   errlen) ||
      !scalar_long(db, "SELECT COUNT(*) FROM public.transactions WHERE status = 'issued'",
                   &active_loans, errbuf, errlen) ||
      !scalar_long(db,
                   "SELECT COUNT(*) FROM public.transactions"
                   " WHERE status = 'issued' AND due_date < CURRENT_DATE",
                   &overdue_loans, errbuf, errlen) ||
      !scalar_long(db, "SELECT COUNT(*) FROM public.fines WHERE status = 'pending'",
                   &pending_fines, errbuf, errlen) ||
      !scalar_long(db,
                   "SELECT COALESCE(SUM(amount), 0) FROM public.fines"
                   " WHERE status = 'pending'",
                   &pending_amount, errbuf, errlen)) {
    return REPORT_QUERY_FAILED;
  }

  DbResult recent;
  if (!db_query(db, RECENT_TX_SQL, NULL, 0, &recent, errbuf, errlen)) {
    return REPORT_QUERY_FAILED;
  }

  char recent_json[8192];
  int written = json_array_of_objects((const char *const *)recent.columns, RECENT_KINDS,
                                      (const char *const *)recent.values, recent.nrows,
                                      recent.ncols, recent_json, sizeof(recent_json));
  db_result_free(&recent);
  if (written < 0) {
    return REPORT_BUFFER_TOO_SMALL;
  }

  int total = snprintf(
      out, out_size,
      "{\"report\":{\"type\":\"overview\","
      "\"total_books\":%ld,\"available_copies\":%ld,\"total_students\":%ld,"
      "\"active_loans\":%ld,\"overdue_loans\":%ld,"
      "\"pending_fines\":%ld,\"pending_fines_amount\":%ld,"
      "\"recent_transactions\":%s}}",
      total_books, available_copies, total_students, active_loans, overdue_loans,
      pending_fines, pending_amount, recent_json);
  if (total < 0 || (size_t)total >= out_size) {
    return REPORT_BUFFER_TOO_SMALL;
  }
  return REPORT_OK;
}