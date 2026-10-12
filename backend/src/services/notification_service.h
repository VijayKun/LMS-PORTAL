/**
 * notification_service.h - Notifications business logic boundary.
 *
 * Read-only for now; creating announcements lands here later (Milestone 4).
 */
#ifndef LMS_NOTIFICATION_SERVICE_H
#define LMS_NOTIFICATION_SERVICE_H

#include <stddef.h>

#include "services/list_service.h"

/**
 * Optional filters for a notifications list. All fields are optional; a zeroed
 * struct (or NULL) means "first page, no filters".
 */
typedef struct {
  const char *user_id; /* exact match on notifications.user_id ('all' = everyone) */
  const char *type;    /* exact match on notifications.type */
  int limit;
  int offset;
} NotificationListQuery;

/**
 * Load a page of notifications from public.notifications, newest first.
 *
 * On LIST_OK the output holds {"notifications":[...],"limit":L,"offset":O}.
 */
ListStatus notification_service_list(Database *db, const NotificationListQuery *query,
                                     char *out, size_t out_size, char *errbuf, size_t errlen);

/**
 * Load a single notification by id.
 *
 * On LIST_OK the output holds {"notification":{...}}; missing ids report
 * LIST_NOT_FOUND.
 */
ListStatus notification_service_get(Database *db, const char *id, char *out, size_t out_size,
                                    char *errbuf, size_t errlen);

#endif /* LMS_NOTIFICATION_SERVICE_H */