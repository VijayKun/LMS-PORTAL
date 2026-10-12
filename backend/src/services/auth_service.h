/**
 * auth_service.h - Login and current-admin lookup.
 */
#ifndef LMS_AUTH_SERVICE_H
#define LMS_AUTH_SERVICE_H

#include <stddef.h>

#include "database/db.h"

typedef enum {
  AUTH_LOGIN_OK = 0,
  AUTH_LOGIN_BAD_CREDENTIALS, /* unknown email or wrong password */
  AUTH_LOGIN_DB_UNAVAILABLE,  /* no database configured/connected */
  AUTH_LOGIN_QUERY_FAILED     /* the query itself failed */
} AuthLoginStatus;

/** Safe (non-secret) admin fields; never includes password_hash. */
typedef struct {
  char id[LMS_MAX_STR];
  char name[LMS_MAX_STR];
  char email[LMS_MAX_STR];
  char role[32];
  char title[LMS_MAX_STR];
} AuthAdmin;

/**
 * Verify credentials against public.admins.
 *
 * On AUTH_LOGIN_OK, out_admin is filled with safe fields. Failures never leak
 * whether the email or the password was wrong - both yield BAD_CREDENTIALS.
 *
 * @param db        database handle
 * @param email     admin email (must not be NULL/empty)
 * @param password  cleartext password
 * @param out_admin receives the admin on success
 * @param errbuf    server-side error detail (log it, don't send it)
 * @param errlen    size of errbuf
 */
AuthLoginStatus auth_service_login(Database *db, const char *email, const char *password,
                                   AuthAdmin *out_admin, char *errbuf, size_t errlen);

typedef enum {
  AUTH_ME_OK = 0,
  AUTH_ME_NOT_FOUND,     /* no admin with the requested id */
  AUTH_ME_DB_UNAVAILABLE, /* no database configured/connected */
  AUTH_ME_QUERY_FAILED   /* the query itself failed */
} AuthMeStatus;

/**
 * Load a single admin by id into out_admin (used for GET /api/auth/me).
 */
AuthMeStatus auth_service_get_admin(Database *db, const char *id, AuthAdmin *out_admin,
                                    char *errbuf, size_t errlen);

#endif /* LMS_AUTH_SERVICE_H */