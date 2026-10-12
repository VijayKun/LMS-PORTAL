/**
 * student_controller.c - Request handling for the student directory.
 *
 * Reads through the service, maps service status to an HTTP response and never
 * leaks connection strings or internal database errors to the client.
 */
#include "controllers/student_controller.h"

#include <stdio.h>
#include <stdlib.h>

#include "services/student_service.h"
#include "utils/response_utils.h"

/* Plenty for the sample directory; larger sets would stream or paginate later. */
#define STUDENTS_JSON_BUFFER_SIZE 65536

void student_controller_list(AppContext *app, struct mg_connection *c,
                             const struct mg_http_message *hm) {
  (void)hm; /* no query parameters yet */

  char *buffer = malloc(STUDENTS_JSON_BUFFER_SIZE);
  if (buffer == NULL) {
    send_json_error(c, 500, "server_error", "The server ran out of memory.");
    return;
  }

  char errbuf[256] = {0};
  StudentListStatus status =
      student_service_list(app ? app->db : NULL, buffer, STUDENTS_JSON_BUFFER_SIZE, errbuf,
                           sizeof(errbuf));

  switch (status) {
    case STUDENT_LIST_OK:
      send_json(c, 200, buffer);
      break;

    case STUDENT_LIST_DB_UNAVAILABLE:
      send_json_error(c, 503, "database_unavailable",
                      "The student directory is temporarily unavailable.");
      break;

    case STUDENT_LIST_BUFFER_TOO_SMALL:
      fprintf(stderr, "[students] response buffer too small for the result set\n");
      send_json_error(c, 500, "response_too_large",
                      "The student list is too large to return.");
      break;

    case STUDENT_LIST_QUERY_FAILED:
    default:
      /* Log server-side only; the client gets a generic message. */
      fprintf(stderr, "[students] query failed: %s\n", errbuf[0] ? errbuf : "(no details)");
      send_json_error(c, 500, "database_error", "Could not load students right now.");
      break;
  }

  free(buffer);
}
