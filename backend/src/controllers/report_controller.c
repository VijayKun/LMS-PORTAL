/**
 * report_controller.c - Request handling for reports.
 *
 * Reports are read-only aggregates; the controller maps ReportStatus to HTTP
 * and never leaks internal database errors to the client.
 */
#include "controllers/report_controller.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "middleware/auth_middleware.h"
#include "services/report_service.h"
#include "utils/response_utils.h"

/* The overview report (counts + up to 10 recent transactions) is small. */
#define REPORT_JSON_BUFFER_SIZE 32768

/* Longest report type we accept before answering 404. */
#define REPORT_TYPE_MAX 64

void report_controller_get(AppContext *app, struct mg_connection *c,
                           const struct mg_http_message *hm, struct mg_str type) {
  Session *session = auth_require_admin(app, c, hm);
  if (session == NULL) {
    return;
  }
  (void)session;

  char typebuf[REPORT_TYPE_MAX];
  if (type.len == 0 || type.len >= sizeof(typebuf)) {
    send_json_error(c, 404, "not_found", "Unknown report type.");
    return;
  }
  memcpy(typebuf, type.buf, type.len);
  typebuf[type.len] = '\0';

  if (report_service_check_type(typebuf) != REPORT_OK) {
    send_json_error(c, 404, "not_found", "Unknown report type.");
    return;
  }

  char *buffer = malloc(REPORT_JSON_BUFFER_SIZE);
  if (buffer == NULL) {
    send_json_error(c, 500, "server_error", "The server ran out of memory.");
    return;
  }

  char errbuf[256] = {0};
  ReportStatus status = report_service_overview(app ? app->db : NULL, buffer,
                                                REPORT_JSON_BUFFER_SIZE, errbuf,
                                                sizeof(errbuf));

  switch (status) {
    case REPORT_OK:
      send_json(c, 200, buffer);
      break;
    case REPORT_TYPE_UNSUPPORTED:
      send_json_error(c, 404, "not_found", "Unknown report type.");
      break;
    case REPORT_DB_UNAVAILABLE:
      send_json_error(c, 503, "database_unavailable", "Reports are temporarily unavailable.");
      break;
    case REPORT_BUFFER_TOO_SMALL:
      fprintf(stderr, "[reports] response buffer too small for '%s'\n", typebuf);
      send_json_error(c, 500, "response_too_large", "The report is too large to return.");
      break;
    case REPORT_QUERY_FAILED:
    default:
      fprintf(stderr, "[reports] '%s' failed: %s\n", typebuf,
              errbuf[0] ? errbuf : "(no details)");
      send_json_error(c, 500, "database_error", "Could not build the report right now.");
      break;
  }

  free(buffer);
}
