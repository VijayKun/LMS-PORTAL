/**
 * branch_service.h - Library branch business logic.
 */
#ifndef LMS_BRANCH_SERVICE_H
#define LMS_BRANCH_SERVICE_H

#include <stddef.h>

#include "services/list_service.h"

ListStatus branch_service_list(Database *db, char *out, size_t out_size, char *errbuf,
                               size_t errlen);

typedef enum {
  BRANCH_OK = 0,
  BRANCH_BAD_INPUT,
  BRANCH_NOT_FOUND,
  BRANCH_CONFLICT,
  BRANCH_DB_UNAVAILABLE,
  BRANCH_QUERY_FAILED
} BranchStatus;

BranchStatus branch_service_create(Database *db, const char *id, const char *name,
                                   char *id_out, size_t id_size, char *errbuf,
                                   size_t errlen);
BranchStatus branch_service_update(Database *db, const char *id, const char *name,
                                   char *errbuf, size_t errlen);
BranchStatus branch_service_delete(Database *db, const char *id, char *errbuf, size_t errlen);

#endif /* LMS_BRANCH_SERVICE_H */
