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

typedef enum {
  STUDENT_GET_OK = 0,
  STUDENT_GET_NOT_FOUND,        /* no student with the requested id */
  STUDENT_GET_DB_UNAVAILABLE,   /* no database configured/connected */
  STUDENT_GET_QUERY_FAILED,     /* the query itself failed */
  STUDENT_GET_BUFFER_TOO_SMALL  /* result did not fit the output buffer */
} StudentGetStatus;

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

/**
 * Load a single student by id as JSON.
 *
 * On STUDENT_GET_OK the output buffer holds {"student":{...}}. A missing id is
 * reported as STUDENT_GET_NOT_FOUND (not an error).
 *
 * @param db       database handle
 * @param id       student id (must not be NULL or empty)
 * @param out      output buffer
 * @param out_size size of out
 * @param errbuf   receives server-side error detail (log it, don't send it)
 * @param errlen   size of errbuf
 */
StudentGetStatus student_service_get(Database *db, const char *id, char *out,
                                     size_t out_size, char *errbuf, size_t errlen);

#endif /* LMS_STUDENT_SERVICE_H */
