/**
 * notification_service.c - Notifications read-only lists.
 */
#include "services/notification_service.h"

#include <string.h>

static const ListSpec NOTIFICATION_SPEC = {
    .response_key = "notifications",
    .columns = "id, type, title, message, time, read, user_id",
    .table = "public.notifications",
    .order_by = "time DESC, id",
    .filter_columns = {"user_id", "type", NULL},
};

ListStatus notification_service_list(Database *db, const NotificationListQuery *query,
                                     char *out, size_t out_size, char *errbuf, size_t errlen) {
  ListQuery q = {0};
  if (query != NULL) {
    q.filter_values[0] = query->user_id;
    q.filter_values[1] = query->type;
    q.limit = query->limit;
    q.offset = query->offset;
  }
  return list_service_list(db, &NOTIFICATION_SPEC, &q, out, out_size, errbuf, errlen);
}

ListStatus notification_service_get(Database *db, const char *id, char *out, size_t out_size,
                                    char *errbuf, size_t errlen) {
  return list_service_get(db, NOTIFICATION_SPEC.table, "id", id, NOTIFICATION_SPEC.columns,
                          NOTIFICATION_SPEC.response_key, out, out_size, errbuf, errlen);
}