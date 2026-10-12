/**
 * session_store.h - In-memory server-side session store.
 *
 * Sessions live only on the server (never in a forgeable client-side blob).
 * The token is an opaque random value; the client only ever holds it in an
 * HttpOnly cookie (or an Authorization header). Expiry is checked lazily on
 * every lookup and proactively on a periodic purge.
 *
 * The server is single-threaded (one Mongoose poll loop), so no locking is
 * needed.
 */
#ifndef LMS_SESSION_STORE_H
#define LMS_SESSION_STORE_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#include "config/config.h"

/** Session token length (raw bytes before hex encoding). */
#define SESSION_TOKEN_BYTES 32

/** Human-readable token length: hex of SESSION_TOKEN_BYTES + NUL. */
#define SESSION_TOKEN_STR (SESSION_TOKEN_BYTES * 2 + 1)

/** Maximum concurrent sessions. */
#define SESSION_STORE_MAX 1024

/** Longest admin id a session may carry (matches LMS_MAX_STR). */
#define SESSION_ADMIN_ID_MAX LMS_MAX_STR

/** A single live session. */
typedef struct {
  bool used;
  char token[SESSION_TOKEN_STR];
  char admin_id[SESSION_ADMIN_ID_MAX];
  char role[32];          /* 'admin' or 'librarian', snapshotted at login */
  time_t created_at;      /* absolute-timeout anchor */
  time_t last_seen_at;    /* idle-timeout anchor */
  long ttl_abs_seconds;   /* created_at + ttl_abs_seconds is the hard drop time */
  long ttl_idle_seconds;  /* last_seen_at + ttl_idle_seconds is the idle drop */
} Session;

/** Fixed-size session table. */
typedef struct {
  Session slots[SESSION_STORE_MAX];
} SessionStore;

/** Reset the store to empty. */
void session_store_init(SessionStore *store);

/**
 * Create and store a session.
 *
 * @param store       session store
 * @param token       caller-generated random token (SESSION_TOKEN_STR bytes)
 * @param admin_id    owning admin id
 * @param role        admin role ('admin' / 'librarian')
 * @param now         current time
 * @param ttl_abs     absolute lifetime in seconds
 * @param ttl_idle    idle timeout in seconds
 * @return pointer to the stored session, or NULL when the table is full.
 */
Session *session_create(SessionStore *store, const char *token, const char *admin_id,
                        const char *role, time_t now, long ttl_abs, long ttl_idle);

/**
 * Look up a session by token, validating both the absolute TTL and the idle
 * TTL. A live session has its last_seen_at refreshed (idle timer reset).
 *
 * @param store session store
 * @param token candidate token (exact match on the stored hex string)
 * @param now   current time
 * @return pointer to the live session, or NULL when absent or expired.
 */
Session *session_lookup(SessionStore *store, const char *token, time_t now);

/** Delete a session (logout / revocation). No-op when the token is absent. */
void session_destroy(SessionStore *store, const char *token);

/** Drop every expired session. Call periodically (e.g. once per auth request). */
void session_purge_expired(SessionStore *store, time_t now);

#endif /* LMS_SESSION_STORE_H */