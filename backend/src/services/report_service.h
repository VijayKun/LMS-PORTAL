/**
 * report_service.h - Reports / dashboard stats business logic boundary.
 */
#ifndef LMS_REPORT_SERVICE_H
#define LMS_REPORT_SERVICE_H

#include <stddef.h>

#include "database/db.h"

typedef enum {
  REPORT_OK = 0,
  REPORT_TYPE_UNSUPPORTED,
  REPORT_DB_UNAVAILABLE,
  REPORT_QUERY_FAILED,
  REPORT_BUFFER_TOO_SMALL
} ReportStatus;

/** Maps a lowercase report type string to a ReportStatus. */
ReportStatus report_service_check_type(const char *type);

/**
 * Produce the "overview" report: headline counts plus the most recent
 * transactions. On REPORT_OK, out holds {"report":{...}}.
 */
ReportStatus report_service_overview(Database *db, char *out, size_t out_size,
                                     char *errbuf, size_t errlen);

#endif /* LMS_REPORT_SERVICE_H */