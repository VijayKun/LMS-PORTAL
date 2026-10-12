/**
 * config.h - Environment-driven configuration.
 *
 * All secrets and environment-specific values are read at startup from
 * environment variables. A local ".env" file is supported for convenience
 * during development, but real environment variables always win.
 *
 * Nothing here should ever hardcode credentials.
 */
#ifndef LMS_CONFIG_H
#define LMS_CONFIG_H

#include <stddef.h>

#define LMS_MAX_STR 256
#define LMS_BACKEND_NAME "lms-backend"
#define LMS_BACKEND_VERSION "0.1.0"

/** PostgreSQL connection settings (used from Milestone 2 onward). */
typedef struct {
  char host[LMS_MAX_STR];
  char port[16];
  char name[LMS_MAX_STR];
  char user[LMS_MAX_STR];
  char password[LMS_MAX_STR];
  char sslmode[32];
} DbConfig;

/** Everything the server needs to boot. */
typedef struct {
  char host[LMS_MAX_STR];
  char port[16];
  char cors_origin[LMS_MAX_STR];
  DbConfig db;
} ServerConfig;

/**
 * Load configuration into cfg.
 *
 * @param cfg      destination (must not be NULL)
 * @param env_path optional path to a ".env" file; pass NULL to skip
 *
 * Precedence: real environment variables override values from the .env file,
 * which override built-in defaults.
 */
void config_load(ServerConfig *cfg, const char *env_path);

/**
 * Print the resolved configuration to stdout. The database password is never
 * printed (it is shown as "(set)" or "(empty)").
 */
void config_print(const ServerConfig *cfg);

#endif /* LMS_CONFIG_H */
