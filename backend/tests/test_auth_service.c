/**
 * test_auth_service.c - Unit tests for the safe-failure behavior of the auth
 * service. No live database is required: only "no database"/"bad input" paths.
 *
 * Run with:  make test
 */
#include <stdio.h>
#include <string.h>

#include "services/auth_service.h"

static int g_failures = 0;

#define CHECK(cond)                                                       \
  do {                                                                    \
    if (!(cond)) {                                                        \
      printf("  FAIL (line %d): %s\n", __LINE__, #cond);                  \
      g_failures++;                                                       \
    }                                                                     \
  } while (0)

static void test_login_null_database_is_unavailable(void) {
  char err[128] = {0};
  AuthAdmin admin;

  AuthLoginStatus s = auth_service_login(NULL, "a@b.c", "pw", &admin, err, sizeof(err));
  CHECK(s == AUTH_LOGIN_DB_UNAVAILABLE);
}

static void test_login_disconnected_database_is_unavailable(void) {
  Database db;
  memset(&db, 0, sizeof(db));
  char err[128] = {0};
  AuthAdmin admin;

  AuthLoginStatus s = auth_service_login(&db, "a@b.c", "pw", &admin, err, sizeof(err));
  CHECK(s == AUTH_LOGIN_DB_UNAVAILABLE);
}

static void test_login_empty_email_is_bad_credentials(void) {
  char err[128] = {0};
  AuthAdmin admin;

  AuthLoginStatus s = auth_service_login(NULL, "", "pw", &admin, err, sizeof(err));
  CHECK(s == AUTH_LOGIN_BAD_CREDENTIALS);
  s = auth_service_login(NULL, NULL, "pw", &admin, err, sizeof(err));
  CHECK(s == AUTH_LOGIN_BAD_CREDENTIALS);
  s = auth_service_login(NULL, "a@b.c", NULL, &admin, err, sizeof(err));
  CHECK(s == AUTH_LOGIN_BAD_CREDENTIALS);
}

static void test_login_null_output_is_failure(void) {
  char err[128] = {0};
  AuthLoginStatus s = auth_service_login(NULL, "a@b.c", "pw", NULL, err, sizeof(err));
  CHECK(s == AUTH_LOGIN_QUERY_FAILED);
}

static void test_me_null_database_is_unavailable(void) {
  char err[128] = {0};
  AuthAdmin admin;

  AuthMeStatus s = auth_service_get_admin(NULL, "a1", &admin, err, sizeof(err));
  CHECK(s == AUTH_ME_DB_UNAVAILABLE);
}

static void test_me_disconnected_database_is_unavailable(void) {
  Database db;
  memset(&db, 0, sizeof(db));
  char err[128] = {0};
  AuthAdmin admin;

  AuthMeStatus s = auth_service_get_admin(&db, "a1", &admin, err, sizeof(err));
  CHECK(s == AUTH_ME_DB_UNAVAILABLE);
}

static void test_me_empty_id_is_not_found(void) {
  char err[128] = {0};
  AuthAdmin admin;

  AuthMeStatus s = auth_service_get_admin(NULL, "", &admin, err, sizeof(err));
  CHECK(s == AUTH_ME_NOT_FOUND);
}

int main(void) {
  printf("Running auth_service tests...\n");
  test_login_null_database_is_unavailable();
  test_login_disconnected_database_is_unavailable();
  test_login_empty_email_is_bad_credentials();
  test_login_null_output_is_failure();
  test_me_null_database_is_unavailable();
  test_me_disconnected_database_is_unavailable();
  test_me_empty_id_is_not_found();

  if (g_failures == 0) {
    printf("All auth_service tests passed.\n");
    return 0;
  }
  printf("%d test(s) failed.\n", g_failures);
  return 1;
}