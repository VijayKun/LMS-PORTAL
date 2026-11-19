/**
 * auth_service.h - Authentication business logic boundary.
 *
 * Admins and students are both authenticated through the same service. The
 * caller receives a role so controllers can enforce authorization without
 * knowing which table the identity came from.
 */
#ifndef LMS_AUTH_SERVICE_H
#define LMS_AUTH_SERVICE_H

#include <stddef.h>

#include "database/db.h"

#define AUTH_MAX_FIELD 256

typedef struct {
  char id[AUTH_MAX_FIELD];
  char name[AUTH_MAX_FIELD];
  char email[AUTH_MAX_FIELD];
  char role[32];  /* "admin", "librarian", or "student" */
  char title[AUTH_MAX_FIELD];
} AuthUser;

/* Backwards-compatible alias used by older callers/tests. */
typedef AuthUser AuthAdmin;

typedef enum {
  AUTH_LOGIN_OK = 0,
  AUTH_LOGIN_BAD_CREDENTIALS,
  AUTH_LOGIN_DB_UNAVAILABLE,
  AUTH_LOGIN_QUERY_FAILED
} AuthLoginStatus;

typedef enum {
  AUTH_ME_OK = 0,
  AUTH_ME_NOT_FOUND,
  AUTH_ME_DB_UNAVAILABLE,
  AUTH_ME_QUERY_FAILED
} AuthMeStatus;

/**
 * Authenticate an admin or student by email (students may also use their
 * student_id). Password hashes are verified with scrypt. On success `out` is
 * filled and its role field is set.
 */
AuthLoginStatus auth_service_login(Database *db, const char *email, const char *password,
                                   AuthUser *out, char *errbuf, size_t errlen);

/**
 * Load an identity by id and role. `role` may be "admin", "librarian", or
 * "student"; unknown roles are treated as admins for backwards compatibility.
 */
AuthMeStatus auth_service_get_user(Database *db, const char *id, const char *role, AuthUser *out,
                                   char *errbuf, size_t errlen);

/* Backwards-compatible admin-only lookup. */
AuthMeStatus auth_service_get_admin(Database *db, const char *id, AuthAdmin *out, char *errbuf,
                                    size_t errlen);

#endif /* LMS_AUTH_SERVICE_H */
