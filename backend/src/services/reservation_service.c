/**
 * reservation_service.c - Reservations lists and queue transitions.
 */
#include "services/reservation_service.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "database/db.h"

static const ListSpec RESERVATION_SPEC = {
    .response_key = "reservations",
    .columns = "id, student_id, book_id, reservation_date, queue_position, status, "
               "expected_availability, pickup_deadline",
    .table = "public.reservations",
    .order_by = "id",
    .filter_columns = {"student_id", "book_id", "status", NULL},
};

ListStatus reservation_service_list(Database *db, const ReservationListQuery *query,
                                    char *out, size_t out_size, char *errbuf, size_t errlen) {
  ListQuery q = {0};
  if (query != NULL) {
    q.filter_values[0] = query->student_id;
    q.filter_values[1] = query->book_id;
    q.filter_values[2] = query->status;
    q.limit = query->limit;
    q.offset = query->offset;
  }
  return list_service_list(db, &RESERVATION_SPEC, &q, out, out_size, errbuf, errlen);
}

ListStatus reservation_service_get(Database *db, const char *id, char *out, size_t out_size,
                                   char *errbuf, size_t errlen) {
  return list_service_get(db, RESERVATION_SPEC.table, "id", id, RESERVATION_SPEC.columns,
                          RESERVATION_SPEC.response_key, out, out_size, errbuf, errlen);
}

ReservationOwnerStatus reservation_service_get_owner(Database *db, const char *id, char *out,
                                                     size_t out_size, char *errbuf,
                                                     size_t errlen) {
  if (id == NULL || id[0] == '\0' || out == NULL || out_size == 0) {
    return RESERVATION_OWNER_NOT_FOUND;
  }
  const char *params[1] = {id};
  DbResult result;
  memset(&result, 0, sizeof(result));
  char db_err[256] = {0};
  if (!db_query(db, "SELECT student_id FROM public.reservations WHERE id = $1 LIMIT 1", params,
                1, &result, db_err, sizeof(db_err))) {
    db_result_free(&result);
    if (db == NULL || !db->connected) {
      return RESERVATION_OWNER_DB_UNAVAILABLE;
    }
    snprintf(errbuf, errlen, "%s", db_err);
    return RESERVATION_OWNER_QUERY_FAILED;
  }
  if (result.nrows == 0) {
    db_result_free(&result);
    return RESERVATION_OWNER_NOT_FOUND;
  }
  const char *owner = result.values[0];
  snprintf(out, out_size, "%s", owner == NULL ? "" : owner);
  db_result_free(&result);
  return RESERVATION_OWNER_OK;
}

/* ---- Date helpers (UTC civil calendrics; see loan_service.c) ------------ */

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

static void format_day(int days, char *buf, size_t size) {
  int year = 0, month = 0, day = 0;
  civil_from_days(days, &year, &month, &day);
  snprintf(buf, size, "%04d-%02d-%02d", year, month, day);
}

static int current_day(void) {
  time_t now = time(NULL);
  return (int)(now / 86400);
}

/* ---- Create ------------------------------------------------------------- */

#define NEXT_QUEUE_SQL                                                      \
  "SELECT COALESCE(MAX(queue_position), 0) + 1 FROM public.reservations"   \
  " WHERE book_id = $1"

#define ACTIVE_RESERVATION_SQL                                                \
  "SELECT id FROM public.reservations"                                        \
  " WHERE student_id = $1 AND book_id = $2 AND status = 'waiting' LIMIT 1"

#define RESERVATION_INSERT_SQL                                                     \
  "INSERT INTO public.reservations"                                                \
  " (id, student_id, book_id, reservation_date, queue_position, status)"           \
  " VALUES (gen_random_uuid()::text, $1, $2, $3, $4, 'waiting') RETURNING id"

ReservationCreateStatus reservation_service_create(Database *db,
                                                   const ReservationInput *input,
                                                   char *id_out, size_t id_size,
                                                   char *errbuf, size_t errlen) {
  if (id_out == NULL || id_size == 0) {
    return RESERVATION_CREATE_BAD_INPUT;
  }
  if (input == NULL || input->student_id[0] == '\0' || input->book_id[0] == '\0') {
    return RESERVATION_CREATE_BAD_INPUT;
  }
  if (!db_is_configured(db)) {
    return RESERVATION_CREATE_DB_UNAVAILABLE;
  }

  const char *params[2];
  DbResult result;

  params[0] = input->student_id;
  params[1] = input->book_id;
  if (!db_query(db, ACTIVE_RESERVATION_SQL, params, 2, &result, errbuf, errlen)) {
    return RESERVATION_CREATE_QUERY_FAILED;
  }
  if (result.nrows > 0) {
    db_result_free(&result);
    return RESERVATION_CREATE_DUPLICATE;
  }
  db_result_free(&result);

  params[0] = input->book_id;
  if (!db_query(db, NEXT_QUEUE_SQL, params, 1, &result, errbuf, errlen)) {
    return RESERVATION_CREATE_QUERY_FAILED;
  }
  int queue_position = 1;
  if (result.nrows == 1 && result.values != NULL && result.values[0] != NULL) {
    queue_position = atoi(result.values[0]);
  }
  db_result_free(&result);
  if (queue_position < 1) {
    queue_position = 1;
  }

  char today[16];
  today_utc_date(today, sizeof(today));
  char queue_buf[16];
  snprintf(queue_buf, sizeof(queue_buf), "%d", queue_position);

  const char *ins[4];
  ins[0] = input->student_id;
  ins[1] = input->book_id;
  ins[2] = today;
  ins[3] = queue_buf;
  if (!db_query(db, RESERVATION_INSERT_SQL, ins, 4, &result, errbuf, errlen)) {
    if (db_has_sqlstate(errbuf, DB_SQLSTATE_FOREIGN_KEY_VIOLATION)) {
      return RESERVATION_CREATE_NOT_FOUND;
    }
    return RESERVATION_CREATE_QUERY_FAILED;
  }

  ReservationCreateStatus status = RESERVATION_CREATE_QUERY_FAILED;
  if (result.nrows == 1 && result.values != NULL && result.values[0] != NULL) {
    snprintf(id_out, id_size, "%s", result.values[0]);
    status = RESERVATION_CREATE_OK;
  }
  db_result_free(&result);
  return status;
}

/* ---- Cancel / ready ----------------------------------------------------- */

#define RESERVATION_CANCEL_SQL                                             \
  "UPDATE public.reservations"                                             \
  " SET status = 'expired', pickup_deadline = NULL"                        \
  " WHERE id = $1 AND status IN ('waiting', 'ready') RETURNING id"

#define RESERVATION_READY_SQL                                                \
  "UPDATE public.reservations"                                                \
  " SET status = 'ready', pickup_deadline = $2, expected_availability = NULL" \
  " WHERE id = $1 AND status = 'waiting' RETURNING id"

ReservationCancelStatus reservation_service_cancel(Database *db, const char *id,
                                                   char *errbuf, size_t errlen) {
  if (id == NULL || id[0] == '\0') {
    return RESERVATION_CANCEL_BAD_INPUT;
  }
  if (!db_is_configured(db)) {
    return RESERVATION_CANCEL_DB_UNAVAILABLE;
  }

  const char *params[1] = {id};
  DbResult result;
  if (!db_query(db, RESERVATION_CANCEL_SQL, params, 1, &result, errbuf, errlen)) {
    return RESERVATION_CANCEL_QUERY_FAILED;
  }

  ReservationCancelStatus status = RESERVATION_CANCEL_NOT_FOUND;
  if (result.nrows == 1) {
    status = RESERVATION_CANCEL_OK;
  }
  db_result_free(&result);
  return status;
}

ReservationReadyStatus reservation_service_ready(Database *db, const char *id,
                                                 char *errbuf, size_t errlen) {
  if (id == NULL || id[0] == '\0') {
    return RESERVATION_READY_BAD_INPUT;
  }
  if (!db_is_configured(db)) {
    return RESERVATION_READY_DB_UNAVAILABLE;
  }

  char deadline[32];
  format_day(current_day() + RESERVATION_PICKUP_DAYS, deadline, sizeof(deadline));

  const char *params[2];
  params[0] = id;
  params[1] = deadline;
  DbResult result;
  if (!db_query(db, RESERVATION_READY_SQL, params, 2, &result, errbuf, errlen)) {
    return RESERVATION_READY_QUERY_FAILED;
  }

  ReservationReadyStatus status = RESERVATION_READY_NOT_FOUND;
  if (result.nrows == 1) {
    status = RESERVATION_READY_OK;
  }
  db_result_free(&result);
  return status;
}