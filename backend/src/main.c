/**
 * main.c - Backend entry point.
 *
 * Loads configuration, prepares the database handle and starts the HTTP
 * server. Keep this file small: wiring only, no business logic.
 */
#include <stdio.h>
#include <string.h>

#include "config/config.h"
#include "database/db.h"
#include "security/password.h"
#include "server/server.h"

int main(int argc, char **argv) {
  /* CLI helper: lms_backend.exe hash <password> -> prints a scrypt hash to
   * paste into the admins.password_hash column (Milestone 3 seeding). */
  if (argc >= 3 && strcmp(argv[1], "hash") == 0) {
    char out[PASSWORD_HASH_MAX];
    if (!password_hash(argv[2], out, sizeof(out))) {
      fprintf(stderr, "[hash] failed to hash the password.\n");
      return 1;
    }
    printf("%s\n", out);
    printf("# UPDATE public.admins SET password_hash = '<paste above>'\n"
           "# WHERE email = 'librarian@college.edu';\n");
    return 0;
  }

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
