/**
 * notification_service.h - Notifications business logic boundary.
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
  const char *user_id; /* include this user's rows and 'all' announcements */
  const char *type;    /* exact match on notifications.type */
  int limit;
  int offset;
} NotificationListQuery;

ListStatus notification_service_list(Database *db, const NotificationListQuery *query,
                                     char *out, size_t out_size, char *errbuf, size_t errlen);

ListStatus notification_service_get(Database *db, const char *id, char *out, size_t out_size,
                                    char *errbuf, size_t errlen);

#define NOTIFICATION_FIELD_MAX 512

typedef struct {
  char type[32];
  char title[NOTIFICATION_FIELD_MAX];
  char message[NOTIFICATION_FIELD_MAX];
  char user_id[128]; /* empty -> "all" */
} NotificationInput;

typedef enum {
  NOTIFICATION_CREATE_OK = 0,
  NOTIFICATION_CREATE_BAD_INPUT,
  NOTIFICATION_CREATE_DB_UNAVAILABLE,
  NOTIFICATION_CREATE_QUERY_FAILED
} NotificationCreateStatus;

NotificationCreateStatus notification_service_create(Database *db,
                                                      const NotificationInput *input,
                                                      char *id_out, size_t id_size,
                                                      char *errbuf, size_t errlen);

typedef enum {
  NOTIFICATION_UPDATE_OK = 0,
  NOTIFICATION_UPDATE_NOT_FOUND,
  NOTIFICATION_UPDATE_BAD_INPUT,
  NOTIFICATION_UPDATE_DB_UNAVAILABLE,
  NOTIFICATION_UPDATE_QUERY_FAILED
} NotificationUpdateStatus;

/**
 * Mark one notification read. When scope_user is non-NULL only that user's
 * rows (and global 'all' announcements) are updated; admins pass NULL.
 */
NotificationUpdateStatus notification_service_mark_read(Database *db, const char *id,
                                                        const char *scope_user,
                                                        char *errbuf, size_t errlen);

/** Mark every unread notification visible to scope_user as read. */
NotificationUpdateStatus notification_service_mark_all_read(Database *db,
                                                            const char *scope_user,
                                                            int *updated_out,
                                                            char *errbuf, size_t errlen);

/** Resolve the notification's user_id, for ownership checks. */
typedef enum {
  NOTIFICATION_OWNER_OK = 0,
  NOTIFICATION_OWNER_NOT_FOUND,
  NOTIFICATION_OWNER_DB_UNAVAILABLE,
  NOTIFICATION_OWNER_QUERY_FAILED
} NotificationOwnerStatus;

NotificationOwnerStatus notification_service_get_owner(Database *db, const char *id,
                                                       char *out, size_t out_size,
                                                       char *errbuf, size_t errlen);

#endif /* LMS_NOTIFICATION_SERVICE_H */
