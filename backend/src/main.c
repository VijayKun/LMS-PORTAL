/**
 * main.c - Backend entry point.
 *
 * Loads configuration, prepares the database handle and starts the HTTP
 * server. Keep this file small: wiring only, no business logic.
 */
#include <stdio.h>

#include "config/config.h"
#include "database/db.h"
#include "server/server.h"

int main(void) {
  ServerConfig cfg;
  Database db;

  /* Flush logs immediately, even when stdout is redirected to a file. */
  setvbuf(stdout, NULL, _IONBF, 0);

  config_load(&cfg, ".env");
  config_print(&cfg);

  db_init(&db, &cfg.db);

  int exit_code = server_run(&cfg, &db);

  db_close(&db);
  printf("[main] stopped (exit code %d)\n", exit_code);
  return exit_code;
}
