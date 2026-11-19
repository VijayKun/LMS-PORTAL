/**
 * server.c - Mongoose event loop, request dispatch and clean shutdown.
 *
 * Routing here is intentionally simple: match the method + URI and hand off to
 * a route module. As more resources are added, each gets its own *_routes.c.
 *
 * The AppContext is passed through Mongoose's per-connection user data
 * (fn_data) so route handlers can reach the database without globals.
 */
#include "server/server.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "middleware/cors_middleware.h"
#include "routes/auth_routes.h"
#include "routes/branch_routes.h"
#include "routes/branch_routes.h"
#include "routes/book_routes.h"
#include "routes/fine_routes.h"
#include "routes/health_routes.h"
#include "routes/loan_routes.h"
#include "routes/notification_routes.h"
#include "routes/report_routes.h"
#include "routes/reservation_routes.h"
#include "routes/student_routes.h"
#include "utils/response_utils.h"

static volatile sig_atomic_t g_stop = 0;

static void handle_signal(int signum) {
  (void)signum;
  g_stop = 1;
}

/* Dispatch a parsed HTTP request to the matching route. */
static void dispatch(struct mg_connection *c, const struct mg_http_message *hm) {
  AppContext *app = (AppContext *)c->fn_data;

  if (cors_handle_preflight(c, hm)) {
    return;
  }
  if (!cors_write_allowed(hm)) {
    send_json_error(c, 403, "forbidden_origin", "This origin may not modify data.");
    return;
  }
  if (health_routes_handle(app, c, hm)) {
    return;
  }
  if (auth_routes_handle(app, c, hm)) {
    return;
  }
  if (book_routes_handle(app, c, hm)) {
    return;
  }
  if (student_routes_handle(app, c, hm)) {
    return;
  }
  if (loan_routes_handle(app, c, hm)) {
    return;
  }
  if (reservation_routes_handle(app, c, hm)) {
    return;
  }
  if (fine_routes_handle(app, c, hm)) {
    return;
  }
  if (notification_routes_handle(app, c, hm)) {
    return;
  }
  if (branch_routes_handle(app, c, hm)) {
    return;
  }
  if (report_routes_handle(app, c, hm)) {
    return;
  }

  if (hm->uri.len >= 4 && memcmp(hm->uri.buf, "/api", 4) == 0) {
    send_json_error(c, 404, "not_found", "No such API endpoint.");
  } else {
    send_json_error(c, 404, "not_found", "Not found.");
  }
}

/* Mongoose event handler. */
static void on_event(struct mg_connection *c, int ev, void *ev_data) {
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *)ev_data;
    dispatch(c, hm);
  }
}

int server_run(const ServerConfig *cfg, Database *db) {
  struct mg_mgr mgr;
  AppContext app;
  char url[LMS_MAX_STR + 32];

  app.cfg = cfg;
  app.db = db;
  session_store_init(&app.sessions);

  cors_init(cfg);
  health_routes_init();

  signal(SIGINT, handle_signal);
  signal(SIGTERM, handle_signal);

  snprintf(url, sizeof(url), "http://%s:%s", cfg->host, cfg->port);

  mg_mgr_init(&mgr);
  mg_log_set(MG_LL_INFO);

  struct mg_connection *listener = mg_http_listen(&mgr, url, on_event, &app);
  if (listener == NULL) {
    fprintf(stderr, "[server] failed to listen on %s\n", url);
    mg_mgr_free(&mgr);
    return 1;
  }

  printf("[server] listening on %s\n", url);
  printf("[server] try: curl %s/api/health\n", url);

  while (!g_stop) {
    mg_mgr_poll(&mgr, 200);
  }

  printf("\n[server] shutting down\n");
  mg_mgr_free(&mgr);
  return 0;
}
