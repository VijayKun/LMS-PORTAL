/**
 * student_service.h - Student directory business logic.
 *
 * Owns the SQL and turns database rows into JSON. Controllers stay thin and
 * never touch libpq or column names.
 */
#ifndef LMS_STUDENT_SERVICE_H
#define LMS_STUDENT_SERVICE_H

#include <stddef.h>

#include "database/db.h"

typedef enum {
  STUDENT_LIST_OK = 0,
  STUDENT_LIST_DB_UNAVAILABLE,  /* no database configured/connected */
  STUDENT_LIST_QUERY_FAILED,    /* the query itself failed */
  STUDENT_LIST_BUFFER_TOO_SMALL /* result did not fit the output buffer */
} StudentListStatus;

/**
 * Load every student from public.students, ordered by name, as JSON.
 *
 * On STUDENT_LIST_OK the output buffer holds {"students":[...]}.
 *
 * @param db       database handle
 * @param out      output buffer
 * @param out_size size of out
 * @param errbuf   receives server-side error detail (log it, don't send it)
 * @param errlen   size of errbuf
 */
StudentListStatus student_service_list(Database *db, char *out, size_t out_size,
                                       char *errbuf, size_t errlen);

#endif /* LMS_STUDENT_SERVICE_H */
