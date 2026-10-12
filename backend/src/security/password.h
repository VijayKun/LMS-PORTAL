/**
 * password.h - Password hashing via OpenSSL scrypt.
 *
 * Format of a stored hash (self-describing, so params can change later):
 *   scrypt$<N>$<r>$<p>$<salt_hex>$<key_hex>
 */
#ifndef LMS_PASSWORD_H
#define LMS_PASSWORD_H

#include <stdbool.h>
#include <stddef.h>

/** Large enough for any hash this module can emit (params bounded below). */
#define PASSWORD_HASH_MAX 192

/**
 * Hash a cleartext password into the self-describing scrypt format above.
 * @param cleartext  password (must not be NULL)
 * @param out        output buffer
 * @param out_size   size of out
 * @return true on success, false when out is too small or the KDF failed.
 */
bool password_hash(const char *cleartext, char *out, size_t out_size);

/**
 * Verify a cleartext password against a stored hash.
 * Comparison is constant-time. Returns false for malformed/bogus hashes.
 * @param cleartext  candidate password
 * @param stored     hash previously produced by password_hash()
 * @return true when the password matches.
 */
bool password_verify(const char *cleartext, const char *stored);

#endif /* LMS_PASSWORD_H */