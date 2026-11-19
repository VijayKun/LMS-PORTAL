/**
 * fine_service.h - Fines business logic boundary.
 */
#ifndef LMS_FINE_SERVICE_H
#define LMS_FINE_SERVICE_H

#include <stddef.h>

#include "services/list_service.h"

/**
 * Optional filters for a fines list. All fields are optional; a zeroed struct
 * (or NULL) means "first page, no filters".
 */
typedef struct {
  const char *student_id; /* exact match on fines.student_id */
  const char *status;     /* exact match on fines.status ('pending'/'paid') */
  int limit;
  int offset;
} FineListQuery;

/**
 * Load a page of fines from public.fines, ordered by id.
 *
 * On LIST_OK the output holds {"fines":[...],"limit":L,"offset":O}.
 */
ListStatus fine_service_list(Database *db, const FineListQuery *query, char *out,
                             size_t out_size, char *errbuf, size_t errlen);

/**
 * Load a single fine by id.
 *
 * On LIST_OK the output holds {"fine":{...}}; missing ids report LIST_NOT_FOUND.
 */
ListStatus fine_service_get(Database *db, const char *id, char *out, size_t out_size,
                            char *errbuf, size_t errlen);

typedef enum {
  FINE_OWNER_OK = 0,
  FINE_OWNER_NOT_FOUND,
  FINE_OWNER_DB_UNAVAILABLE,
  FINE_OWNER_QUERY_FAILED
} FineOwnerStatus;

/** Resolve the student id that owns a fine, for self-authorization. */
FineOwnerStatus fine_service_get_owner(Database *db, const char *id, char *out,
                                       size_t out_size, char *errbuf, size_t errlen);

typedef enum {
  FINE_PAY_OK = 0,          /* marked 'paid' with today's date */
  FINE_PAY_NOT_FOUND,       /* no pending fine with this id */
  FINE_PAY_BAD_INPUT,       /* empty id */
  FINE_PAY_DB_UNAVAILABLE,  /* no database configured/connected */
  FINE_PAY_QUERY_FAILED     /* the query itself failed */
} FinePayStatus;

/** Mark a pending fine as paid (idempotent when already paid). */
FinePayStatus fine_service_pay(Database *db, const char *id, char *errbuf, size_t errlen);

#endif /* LMS_FINE_SERVICE_H */