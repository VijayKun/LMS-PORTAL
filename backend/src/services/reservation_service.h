/**
 * reservation_service.h - Reservations business logic boundary.
 */
#ifndef LMS_RESERVATION_SERVICE_H
#define LMS_RESERVATION_SERVICE_H

#include <stddef.h>

#include "services/list_service.h"

/** How many days a 'ready' reservation stays available for pickup. */
#define RESERVATION_PICKUP_DAYS 3

/**
 * Optional filters for a reservations list. All fields are optional; a zeroed
 * struct (or NULL) means "first page, no filters".
 */
typedef struct {
  const char *student_id; /* exact match on reservations.student_id */
  const char *book_id;    /* exact match on reservations.book_id */
  const char *status;     /* exact match ('waiting'/'ready'/'fulfilled'/'expired') */
  int limit;
  int offset;
} ReservationListQuery;

/**
 * Load a page of reservations from public.reservations, ordered by id.
 *
 * On LIST_OK the output holds {"reservations":[...],"limit":L,"offset":O}.
 */
ListStatus reservation_service_list(Database *db, const ReservationListQuery *query,
                                    char *out, size_t out_size, char *errbuf, size_t errlen);

/**
 * Load a single reservation by id.
 *
 * On LIST_OK the output holds {"reservation":{...}}; missing ids report
 * LIST_NOT_FOUND.
 */
ListStatus reservation_service_get(Database *db, const char *id, char *out, size_t out_size,
                                   char *errbuf, size_t errlen);

/** Maximum length of a reservation input field. */
#define RESERVATION_FIELD_MAX 128

/**
 * Fields accepted when placing a reservation. Queue position is derived from
 * existing waiting reservations for the same book.
 */
typedef struct {
  char student_id[RESERVATION_FIELD_MAX];
  char book_id[RESERVATION_FIELD_MAX];
} ReservationInput;

typedef enum {
  RESERVATION_CREATE_OK = 0,
  RESERVATION_CREATE_BAD_INPUT,    /* missing ids */
  RESERVATION_CREATE_DUPLICATE,    /* student already waiting on this book */
  RESERVATION_CREATE_NOT_FOUND,    /* student/book does not exist */
  RESERVATION_CREATE_DB_UNAVAILABLE,
  RESERVATION_CREATE_QUERY_FAILED
} ReservationCreateStatus;

/**
 * Create a waiting reservation, taking the next queue position for the book.
 * On RESERVATION_CREATE_OK, id_out receives the reservation id.
 */
ReservationCreateStatus reservation_service_create(Database *db,
                                                   const ReservationInput *input,
                                                   char *id_out, size_t id_size,
                                                   char *errbuf, size_t errlen);

typedef enum {
  RESERVATION_CANCEL_OK = 0,  /* moved to 'expired' */
  RESERVATION_CANCEL_NOT_FOUND,
  RESERVATION_CANCEL_BAD_INPUT,
  RESERVATION_CANCEL_DB_UNAVAILABLE,
  RESERVATION_CANCEL_QUERY_FAILED
} ReservationCancelStatus;

/** Cancel a waiting/ready reservation: marks it 'expired'. */
ReservationCancelStatus reservation_service_cancel(Database *db, const char *id,
                                                   char *errbuf, size_t errlen);

typedef enum {
  RESERVATION_READY_OK = 0,  /* moved to 'ready' with a pickup deadline */
  RESERVATION_READY_NOT_FOUND,
  RESERVATION_READY_BAD_INPUT,
  RESERVATION_READY_DB_UNAVAILABLE,
  RESERVATION_READY_QUERY_FAILED
} ReservationReadyStatus;

/** Mark a waiting reservation 'ready' and set its pickup deadline. */
ReservationReadyStatus reservation_service_ready(Database *db, const char *id,
                                                 char *errbuf, size_t errlen);

#endif /* LMS_RESERVATION_SERVICE_H */