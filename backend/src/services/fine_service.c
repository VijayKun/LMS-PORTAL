/**
 * fine_service.c - Fines lists and collection (pay).
 */
#include "services/fine_service.h"

#include <string.h>
#include <time.h>

#include "database/db.h"

#include <stdio.h>

static const ListSpec FINE_SPEC = {
    .response_key = "fines",
    .columns = "id, student_id, book_id, due_date, return_date, overdue_days, amount, "
               "status, paid_date",
    .table = "public.fines",
    .order_by = "id",
    .filter_columns = {"student_id", "status", NULL},
};

ListStatus fine_service_list(Database *db, const FineListQuery *query, char *out,
                             size_t out_size, char *errbuf, size_t errlen) {
  ListQuery q = {0};
  if (query != NULL) {
    q.filter_values[0] = query->student_id;
    q.filter_values[1] = query->status;
    q.limit = query->limit;
    q.offset = query->offset;
  }
  return list_service_list(db, &FINE_SPEC, &q, out, out_size, errbuf, errlen);
}

ListStatus fine_service_get(Database *db, const char *id, char *out, size_t out_size,
                            char *errbuf, size_t errlen) {
  return list_service_get(db, FINE_SPEC.table, "id", id, FINE_SPEC.columns,
                          FINE_SPEC.response_key, out, out_size, errbuf, errlen);
}

FineOwnerStatus fine_service_get_owner(Database *db, const char *id, char *out,
                                       size_t out_size, char *errbuf, size_t errlen) {
  if (id == NULL || id[0] == '\0' || out == NULL || out_size == 0) {
    return FINE_OWNER_NOT_FOUND;
  }
  const char *params[1] = {id};
  DbResult result;
  memset(&result, 0, sizeof(result));
  char db_err[256] = {0};
  if (!db_query(db, "SELECT student_id FROM public.fines WHERE id = $1 LIMIT 1", params, 1,
                &result, db_err, sizeof(db_err))) {
    db_result_free(&result);
    if (db == NULL || !db->connected) {
      return FINE_OWNER_DB_UNAVAILABLE;
    }
    snprintf(errbuf, errlen, "%s", db_err);
    return FINE_OWNER_QUERY_FAILED;
  }
  if (result.nrows == 0) {
    db_result_free(&result);
    return FINE_OWNER_NOT_FOUND;
  }
  const char *owner = result.values[0];
  snprintf(out, out_size, "%s", owner == NULL ? "" : owner);
  db_result_free(&result);
  return FINE_OWNER_OK;
}

/* ---- Pay ------------------------------------------------------------------ */

#define FINE_PAY_SQL                                  \
  "UPDATE public.fines"                              \
  " SET status = 'paid', paid_date = $2"             \
  " WHERE id = $1 AND status = 'pending' RETURNING id"

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

FinePayStatus fine_service_pay(Database *db, const char *id, char *errbuf, size_t errlen) {
  if (id == NULL || id[0] == '\0') {
    return FINE_PAY_BAD_INPUT;
  }
  if (!db_is_configured(db)) {
    return FINE_PAY_DB_UNAVAILABLE;
  }

  char today[16];
  today_utc_date(today, sizeof(today));

  const char *params[2];
  params[0] = id;
  params[1] = today;
  DbResult result;
  if (!db_query(db, FINE_PAY_SQL, params, 2, &result, errbuf, errlen)) {
    return FINE_PAY_QUERY_FAILED;
  }

  FinePayStatus status = FINE_PAY_NOT_FOUND;
  if (result.nrows == 1) {
    status = FINE_PAY_OK;
  }
  db_result_free(&result);
  return status;
}