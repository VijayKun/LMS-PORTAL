/**
 * session_store.c - See session_store.h.
 */
#include "auth/session_store.h"

#include <stdio.h>
#include <string.h>

void session_store_init(SessionStore *store) {
  if (store == NULL) {
    return;
  }
  memset(store, 0, sizeof(*store));
}

Session *session_create(SessionStore *store, const char *token, const char *admin_id,
                        const char *role, time_t now, long ttl_abs, long ttl_idle) {
  if (store == NULL || token == NULL || admin_id == NULL || role == NULL ||
      token[0] == '\0') {
    return NULL;
  }

  for (size_t i = 0; i < SESSION_STORE_MAX; i++) {
    if (store->slots[i].used) {
      continue;
    }
    Session *s = &store->slots[i];
    memset(s, 0, sizeof(*s));
    s->used = true;
    snprintf(s->token, sizeof(s->token), "%s", token);
    snprintf(s->admin_id, sizeof(s->admin_id), "%s", admin_id);
    snprintf(s->role, sizeof(s->role), "%s", role);
    s->created_at = now;
    s->last_seen_at = now;
    s->ttl_abs_seconds = ttl_abs > 0 ? ttl_abs : 0;
    s->ttl_idle_seconds = ttl_idle > 0 ? ttl_idle : 0;
    return s;
  }
  return NULL; /* table full */
}

static bool session_expired(const Session *s, time_t now) {
  if (s->ttl_abs_seconds > 0 && now - s->created_at >= s->ttl_abs_seconds) {
    return true;
  }
  if (s->ttl_idle_seconds > 0 && now - s->last_seen_at >= s->ttl_idle_seconds) {
    return true;
  }
  return false;
}

Session *session_lookup(SessionStore *store, const char *token, time_t now) {
  if (store == NULL || token == NULL || token[0] == '\0') {
    return NULL;
  }

  for (size_t i = 0; i < SESSION_STORE_MAX; i++) {
    Session *s = &store->slots[i];
    if (!s->used) {
      continue;
    }
    if (strcmp(s->token, token) != 0) {
      continue;
    }
    if (session_expired(s, now)) {
      s->used = false; /* expired: reclaim the slot */
      return NULL;
    }
    s->last_seen_at = now; /* idle timer resets on every use */
    return s;
  }
  return NULL;
}

void session_destroy(SessionStore *store, const char *token) {
  if (store == NULL || token == NULL || token[0] == '\0') {
    return;
  }
  for (size_t i = 0; i < SESSION_STORE_MAX; i++) {
    if (store->slots[i].used && strcmp(store->slots[i].token, token) == 0) {
      store->slots[i].used = false;
      return;
    }
  }
}

void session_purge_expired(SessionStore *store, time_t now) {
  if (store == NULL) {
    return;
  }
  for (size_t i = 0; i < SESSION_STORE_MAX; i++) {
    if (store->slots[i].used && session_expired(&store->slots[i], now)) {
      store->slots[i].used = false;
    }
  }
}