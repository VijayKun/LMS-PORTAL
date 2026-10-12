/**
 * test_session_store.c - Unit tests for the in-memory session store.
 * Run with:  make test
 */
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "auth/session_store.h"

static int g_failures = 0;

#define CHECK(cond)                                                       \
  do {                                                                    \
    if (!(cond)) {                                                        \
      printf("  FAIL (line %d): %s\n", __LINE__, #cond);                  \
      g_failures++;                                                       \
    }                                                                     \
  } while (0)

static void test_create_and_lookup(void) {
  SessionStore store;
  session_store_init(&store);

  Session *s = session_create(&store, "token1", "a1", "admin", 1000, 100, 50);
  CHECK(s != NULL);
  CHECK(s->used);

  Session *found = session_lookup(&store, "token1", 1005);
  CHECK(found == s);
  /* Idle timer refreshes on lookup: this lookup is now valid again. */
  CHECK(found != NULL);
}

static void test_create_rejects_bad_args(void) {
  SessionStore store;
  session_store_init(&store);
  CHECK(session_create(&store, NULL, "a1", "admin", 0, 1, 1) == NULL);
  CHECK(session_create(&store, "", "a1", "admin", 0, 1, 1) == NULL);
  CHECK(session_create(&store, "t", NULL, "admin", 0, 1, 1) == NULL);
  CHECK(session_create(NULL, "t", "a", "admin", 0, 1, 1) == NULL);
}

static void test_unknown_token_is_rejected(void) {
  SessionStore store;
  session_store_init(&store);
  session_create(&store, "tokenA", "a1", "admin", 1000, 100, 50);

  CHECK(session_lookup(&store, "tokenB", 1005) == NULL);
  CHECK(session_lookup(&store, "", 1005) == NULL);
  CHECK(session_lookup(&store, NULL, 1005) == NULL);
}

static void test_absolute_ttl_expiry(void) {
  SessionStore store;
  session_store_init(&store);
  session_create(&store, "abs", "a1", "admin", 1000, /*ttl_abs*/ 100, /*ttl_idle*/ 500);

  CHECK(session_lookup(&store, "abs", 1099) != NULL);  /* inside absolute TTL */
  CHECK(session_lookup(&store, "abs", 1100) == NULL);  /* at/after absolute TTL */
}

static void test_idle_ttl_expiry(void) {
  SessionStore store;
  session_store_init(&store);
  session_create(&store, "idle2", "a1", "admin", 1000, /*ttl_abs*/ 10000, /*ttl_idle*/ 50);

  CHECK(session_lookup(&store, "idle2", 1049) != NULL); /* 49s idle: valid */

  /* Fresh session (no prior lookup that would refresh the idle timer). */
  session_create(&store, "idle", "a1", "admin", 1000, /*ttl_abs*/ 10000, /*ttl_idle*/ 50);
  CHECK(session_lookup(&store, "idle", 1050) == NULL); /* 50s idle: expired */
}

static void test_destroy(void) {
  SessionStore store;
  session_store_init(&store);
  session_create(&store, "rm", "a1", "admin", 1000, 100, 50);

  CHECK(session_lookup(&store, "rm", 1005) != NULL);
  session_destroy(&store, "rm");
  CHECK(session_lookup(&store, "rm", 1005) == NULL);
  session_destroy(&store, "rm"); /* idempotent */
}

static void test_purge_removes_expired(void) {
  SessionStore store;
  session_store_init(&store);
  session_create(&store, "expired", "a1", "admin", 1000, 10, 10);
  session_create(&store, "live", "a2", "librarian", 1000, 5000, 5000);

  session_purge_expired(&store, 2000);
  CHECK(session_lookup(&store, "expired", 2000) == NULL);
  CHECK(session_lookup(&store, "live", 2000) != NULL);
}

int main(void) {
  printf("Running session_store tests...\n");
  test_create_and_lookup();
  test_create_rejects_bad_args();
  test_unknown_token_is_rejected();
  test_absolute_ttl_expiry();
  test_idle_ttl_expiry();
  test_destroy();
  test_purge_removes_expired();

  if (g_failures == 0) {
    printf("All session_store tests passed.\n");
    return 0;
  }
  printf("%d test(s) failed.\n", g_failures);
  return 1;
}