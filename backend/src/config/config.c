/**
 * config.c - Reads configuration from environment variables (and an optional
 * .env file) into a ServerConfig struct.
 */
#include "config/config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ENV_ENTRIES 64
#define MAX_LINE 512

/* Small in-memory copy of a .env file so that real env vars can still win. */
static char g_env_keys[MAX_ENV_ENTRIES][LMS_MAX_STR];
static char g_env_vals[MAX_ENV_ENTRIES][LMS_MAX_STR];
static int g_env_count = 0;

/* Trim leading/trailing whitespace in place. */
static void trim(char *s) {
  if (s == NULL) return;

  char *start = s;
  while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n') {
    start++;
  }

  if (start != s) {
    memmove(s, start, strlen(start) + 1);
  }

  size_t len = strlen(s);
  while (len > 0) {
    char c = s[len - 1];
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
      s[--len] = '\0';
    } else {
      break;
    }
  }
}

/* Remove a matched pair of surrounding single or double quotes. */
static void strip_quotes(char *s) {
  size_t len = strlen(s);
  if (len >= 2) {
    char first = s[0];
    char last = s[len - 1];
    if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
      s[len - 1] = '\0';
      memmove(s, s + 1, len - 1);
    }
  }
}

/* Parse a simple KEY=VALUE .env file. Missing files are not an error. */
static void load_env_file(const char *path) {
  FILE *file = fopen(path, "r");
  if (file == NULL) {
    return;
  }

  char line[MAX_LINE];
  while (fgets(line, (int)sizeof(line), file) != NULL && g_env_count < MAX_ENV_ENTRIES) {
    trim(line);
    if (line[0] == '\0' || line[0] == '#') {
      continue;
    }

    char *eq = strchr(line, '=');
    if (eq == NULL) {
      continue;
    }
    *eq = '\0';

    char *key = line;
    char *value = eq + 1;
    trim(key);
    trim(value);
    strip_quotes(value);

    if (key[0] == '\0') {
      continue;
    }

    snprintf(g_env_keys[g_env_count], LMS_MAX_STR, "%.*s", LMS_MAX_STR - 1, key);
    snprintf(g_env_vals[g_env_count], LMS_MAX_STR, "%.*s", LMS_MAX_STR - 1, value);
    g_env_count++;
  }

  fclose(file);
}

/*
 * Look up a configuration value.
 * Order: real environment variable -> .env file -> fallback.
 */
static const char *cfg_get(const char *key, const char *fallback) {
  const char *value = getenv(key);
  if (value != NULL && value[0] != '\0') {
    return value;
  }

  for (int i = 0; i < g_env_count; i++) {
    if (strcmp(g_env_keys[i], key) == 0) {
      return g_env_vals[i];
    }
  }

  return fallback;
}

void config_load(ServerConfig *cfg, const char *env_path) {
  if (cfg == NULL) {
    return;
  }

  memset(cfg, 0, sizeof(*cfg));
  g_env_count = 0;

  if (env_path != NULL) {
    load_env_file(env_path);
  }

  snprintf(cfg->host, sizeof(cfg->host), "%s", cfg_get("LMS_SERVER_HOST", "127.0.0.1"));
  snprintf(cfg->port, sizeof(cfg->port), "%s", cfg_get("LMS_SERVER_PORT", "8080"));
  snprintf(cfg->cors_origin, sizeof(cfg->cors_origin), "%s",
           cfg_get("LMS_CORS_ALLOWED_ORIGIN", "http://localhost:5173"));

  snprintf(cfg->db.host, sizeof(cfg->db.host), "%s", cfg_get("LMS_DB_HOST", ""));
  snprintf(cfg->db.port, sizeof(cfg->db.port), "%s", cfg_get("LMS_DB_PORT", "5432"));
  snprintf(cfg->db.name, sizeof(cfg->db.name), "%s", cfg_get("LMS_DB_NAME", "postgres"));
  snprintf(cfg->db.user, sizeof(cfg->db.user), "%s", cfg_get("LMS_DB_USER", ""));
  snprintf(cfg->db.password, sizeof(cfg->db.password), "%s", cfg_get("LMS_DB_PASSWORD", ""));
  snprintf(cfg->db.sslmode, sizeof(cfg->db.sslmode), "%s", cfg_get("LMS_DB_SSLMODE", "require"));

  cfg->session_ttl_seconds = strtol(cfg_get("LMS_SESSION_TTL_SECONDS", "43200"), NULL, 10);
  cfg->session_ttl_idle_seconds =
      strtol(cfg_get("LMS_SESSION_TTL_IDLE_SECONDS", "1800"), NULL, 10);

  const char *secure = cfg_get("LMS_COOKIE_SECURE", "0");
  cfg->cookie_secure = (strcmp(secure, "1") == 0 || strcmp(secure, "true") == 0 ||
                        strcmp(secure, "yes") == 0);
}

void config_print(const ServerConfig *cfg) {
  if (cfg == NULL) {
    return;
  }

  printf("[config] %s v%s\n", LMS_BACKEND_NAME, LMS_BACKEND_VERSION);
  printf("[config] server      : http://%s:%s\n", cfg->host, cfg->port);
  printf("[config] cors origin : %s\n", cfg->cors_origin);
  printf("[config] db host     : %s\n", cfg->db.host[0] ? cfg->db.host : "(not set)");
  printf("[config] db port     : %s\n", cfg->db.port);
  printf("[config] db name     : %s\n", cfg->db.name);
  printf("[config] db user     : %s\n", cfg->db.user[0] ? cfg->db.user : "(not set)");
  printf("[config] db password : %s\n", cfg->db.password[0] ? "(set)" : "(empty)");
  printf("[config] db sslmode  : %s\n", cfg->db.sslmode);
  printf("[config] session ttl : %lds absolute, %lds idle; cookie secure=%s\n",
         cfg->session_ttl_seconds, cfg->session_ttl_idle_seconds,
         cfg->cookie_secure ? "yes" : "no");
}
