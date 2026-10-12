/**
 * test_password.c - Unit tests for the scrypt password hashing.
 * Run with:  make test
 */
#include <stdio.h>
#include <string.h>

#include "security/password.h"

static int g_failures = 0;

#define CHECK(cond)                                                       \
  do {                                                                    \
    if (!(cond)) {                                                        \
      printf("  FAIL (line %d): %s\n", __LINE__, #cond);                  \
      g_failures++;                                                       \
    }                                                                     \
  } while (0)

static void test_hash_verify_round_trip(void) {
  char hash[PASSWORD_HASH_MAX];
  CHECK(password_hash("correct horse battery staple", hash, sizeof(hash)));
  CHECK(password_verify("correct horse battery staple", hash));
}

static void test_wrong_password_is_rejected(void) {
  char hash[PASSWORD_HASH_MAX];
  CHECK(password_hash("secret123", hash, sizeof(hash)));
  CHECK(!password_verify("secret124", hash));
}

static void test_empty_password_round_trip(void) {
  char hash[PASSWORD_HASH_MAX];
  CHECK(password_hash("", hash, sizeof(hash)));
  CHECK(password_verify("", hash));
}

static void test_hashes_are_unique_per_salt(void) {
  char a[PASSWORD_HASH_MAX], b[PASSWORD_HASH_MAX];
  CHECK(password_hash("same-password", a, sizeof(a)));
  CHECK(password_hash("same-password", b, sizeof(b)));
  CHECK(strcmp(a, b) != 0);
}

static void test_hash_has_expected_prefix(void) {
  char hash[PASSWORD_HASH_MAX];
  CHECK(password_hash("x", hash, sizeof(hash)));
  CHECK(strncmp(hash, "scrypt$", 7) == 0);
}

static void test_verify_rejects_malformed_stored(void) {
  CHECK(!password_verify("pw", "not-a-scrypt-hash"));
  CHECK(!password_verify("pw", NULL));
  CHECK(!password_verify(NULL, "scrypt$2$1$1$ab$cd"));
}

static void test_verify_rejects_truncated_buffers(void) {
  char tiny[8];
  CHECK(!password_hash("password", tiny, sizeof(tiny)));
}

int main(void) {
  printf("Running password tests...\n");
  test_hash_verify_round_trip();
  test_wrong_password_is_rejected();
  test_empty_password_round_trip();
  test_hashes_are_unique_per_salt();
  test_hash_has_expected_prefix();
  test_verify_rejects_malformed_stored();
  test_verify_rejects_truncated_buffers();

  if (g_failures == 0) {
    printf("All password tests passed.\n");
    return 0;
  }
  printf("%d test(s) failed.\n", g_failures);
  return 1;
}