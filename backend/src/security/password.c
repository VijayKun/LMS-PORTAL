/**
 * password.c - Password hashing using OpenSSL's scrypt KDF.
 *
 * scrypt is a memory-hard, salt-free-of-precomputation KDF comparable to bcrypt
 * or Argon2 for this purpose. Each stored hash carries its own parameters and a
 * random per-user salt, so future hash upgrades only need a new prefix format.
 */
#include "security/password.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/crypto.h> /* CRYPTO_memcmp */
#include <openssl/evp.h>
#include <openssl/rand.h> /* RAND_bytes */

/* scrypt parameters (N=2^14, r=8, p=1 is a bcrypt-comparable work factor). */
#define SCRYPT_N 16384u
#define SCRYPT_R 8u
#define SCRYPT_P 1u

/* 128 * N * r bytes of working memory; give the KDF a little headroom. */
#define SCRYPT_MAXMEM (128u * SCRYPT_N * SCRYPT_R * 2u)

#define SCRYPT_SALT_LEN 16u
#define SCRYPT_KEY_LEN 32u

#define HEX_LEN(n) ((size_t)(n) * 2u + 1u)

/* Small hex encode/decode for the salt and derived key. */
static void to_hex(const unsigned char *bin, size_t len, char *hex) {
  static const char digits[] = "0123456789abcdef";
  for (size_t i = 0; i < len; i++) {
    hex[2 * i] = digits[bin[i] >> 4];
    hex[2 * i + 1] = digits[bin[i] & 0x0f];
  }
  hex[2 * len] = '\0';
}

static int hex_digit(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

static bool from_hex(const char *hex, unsigned char *bin, size_t len) {
  size_t slen = strlen(hex);
  if (slen != len * 2u) {
    return false;
  }
  for (size_t i = 0; i < len; i++) {
    int hi = hex_digit(hex[2 * i]);
    int lo = hex_digit(hex[2 * i + 1]);
    if (hi < 0 || lo < 0) {
      return false;
    }
    bin[i] = (unsigned char)((hi << 4) | lo);
  }
  return true;
}

bool password_hash(const char *cleartext, char *out, size_t out_size) {
  if (cleartext == NULL || out == NULL || out_size == 0) {
    return false;
  }

  /* "scrypt$16384$8$1$<32 hex>$<64 hex>" fits well inside PASSWORD_HASH_MAX. */
  size_t needed = strlen("scrypt$16384$8$1$") + HEX_LEN(SCRYPT_SALT_LEN) + 1 +
                  HEX_LEN(SCRYPT_KEY_LEN) + 1;
  if (out_size < needed) {
    return false;
  }

  unsigned char salt[SCRYPT_SALT_LEN];
  unsigned char key[SCRYPT_KEY_LEN];
  if (!RAND_bytes(salt, (int)SCRYPT_SALT_LEN)) {
    return false;
  }

  if (EVP_PBE_scrypt(cleartext, strlen(cleartext), salt, SCRYPT_SALT_LEN, SCRYPT_N,
                     SCRYPT_R, SCRYPT_P, SCRYPT_MAXMEM, key, SCRYPT_KEY_LEN) != 1) {
    return false;
  }

  char salt_hex[HEX_LEN(SCRYPT_SALT_LEN)];
  char key_hex[HEX_LEN(SCRYPT_KEY_LEN)];
  to_hex(salt, SCRYPT_SALT_LEN, salt_hex);
  to_hex(key, SCRYPT_KEY_LEN, key_hex);

  snprintf(out, out_size, "scrypt$%u$%u$%u$%s$%s", (unsigned)SCRYPT_N, (unsigned)SCRYPT_R,
           (unsigned)SCRYPT_P, salt_hex, key_hex);
  return true;
}

bool password_verify(const char *cleartext, const char *stored) {
  if (cleartext == NULL || stored == NULL) {
    return false;
  }

  unsigned int N = 0, r = 0, p = 0;
  char salt_hex[HEX_LEN(SCRYPT_SALT_LEN)];
  char key_hex[HEX_LEN(SCRYPT_KEY_LEN)];

  int parsed = sscanf(stored, "scrypt$%u$%u$%u$%32s$%64s", &N, &r, &p, salt_hex, key_hex);
  if (parsed != 5) {
    return false;
  }

  unsigned char salt[SCRYPT_SALT_LEN];
  unsigned char expected[SCRYPT_KEY_LEN];
  if (!from_hex(salt_hex, salt, SCRYPT_SALT_LEN) ||
      !from_hex(key_hex, expected, SCRYPT_KEY_LEN)) {
    return false;
  }

  unsigned char actual[SCRYPT_KEY_LEN];
  if (EVP_PBE_scrypt(cleartext, strlen(cleartext), salt, SCRYPT_SALT_LEN, N, r, p,
                     SCRYPT_MAXMEM, actual, SCRYPT_KEY_LEN) != 1) {
    return false;
  }

  return CRYPTO_memcmp(actual, expected, SCRYPT_KEY_LEN) == 0;
}