/**
 * server.h - HTTP server bootstrap (Mongoose) and shared request context.
 */
#ifndef LMS_SERVER_H
#define LMS_SERVER_H

#include "auth/session_store.h"
#include "config/config.h"
#include "database/db.h"

/**
 * Shared application context passed to every route handler.
 * Kept intentionally small: add fields here as the app grows.
 */
typedef struct {
  const ServerConfig *cfg;
  Database *db;
  SessionStore sessions; /* server-side login sessions */
} AppContext;

/**
 * Start the HTTP server and block until interrupted (Ctrl+C).
 * @param cfg resolved configuration
 * @param db  database handle (may be unconnected; endpoints fail safely)
 * @return process exit code (0 on clean shutdown).
 */
int server_run(const ServerConfig *cfg, Database *db);

#endif /* LMS_SERVER_H */
