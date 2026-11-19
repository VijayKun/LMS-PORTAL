/**
 * branch_service.c - Library branch lists and CRUD.
 */
#include "services/branch_service.h"

#include <stdio.h>
#include <string.h>

static const ListSpec BRANCH_SPEC = {
    .response_key = "branches",
    .columns = "id, name",
    .table = "public.branches",
    .order_by = "name",
    .filter_columns = {NULL},
};

ListStatus branch_service_list(Database *db, char *out, size_t out_size, char *errbuf,
                               size_t errlen) {
  ListQuery query = {0};
  query.limit = LIST_MAX_LIMIT;
  return list_service_list(db, &BRANCH_SPEC, &query, out, out_size, errbuf, errlen);
}

BranchStatus branch_service_create(Database *db, const char *id, const char *name,
                                   char *id_out, size_t id_size, char *errbuf,
                                   size_t errlen) {
  if (name == NULL || name[0] == '\0' || id_out == NULL || id_size == 0) {
    return BRANCH_BAD_INPUT;
  }
  if (db == NULL || !db->connected) {
    return BRANCH_DB_UNAVAILABLE;
  }

  const char *params[2] = {id, name};
  DbResult result;
  memset(&result, 0, sizeof(result));
  const char *sql =
      "INSERT INTO public.branches (id, name) VALUES "
      "(CASE WHEN $1 <> '' THEN $1 ELSE lower(regexp_replace($2, '[^a-zA-Z0-9]+', '-', 'g')) "
      " || '-' || substr(md5(random()::text), 1, 6) END, $2) RETURNING id";
  if (!db_query(db, sql, params, 2, &result, errbuf, errlen)) {
    BranchStatus status = db_has_sqlstate(errbuf, DB_SQLSTATE_UNIQUE_VIOLATION)
                              ? BRANCH_CONFLICT
                              : BRANCH_QUERY_FAILED;
    db_result_free(&result);
    return status;
  }
  snprintf(id_out, id_size, "%s", result.values[0] == NULL ? "" : result.values[0]);
  db_result_free(&result);
  return BRANCH_OK;
}

BranchStatus branch_service_update(Database *db, const char *id, const char *name,
                                   char *errbuf, size_t errlen) {
  if (id == NULL || id[0] == '\0' || name == NULL || name[0] == '\0') {
    return BRANCH_BAD_INPUT;
  }
  if (db == NULL || !db->connected) {
    return BRANCH_DB_UNAVAILABLE;
  }
  const char *params[2] = {name, id};
  DbResult result;
  memset(&result, 0, sizeof(result));
  if (!db_query(db, "UPDATE public.branches SET name = $1 WHERE id = $2 RETURNING id", params,
                2, &result, errbuf, errlen)) {
    db_result_free(&result);
    return BRANCH_QUERY_FAILED;
  }
  BranchStatus status = result.nrows == 0 ? BRANCH_NOT_FOUND : BRANCH_OK;
  db_result_free(&result);
  return status;
}

BranchStatus branch_service_delete(Database *db, const char *id, char *errbuf,
                                   size_t errlen) {
  if (id == NULL || id[0] == '\0') {
    return BRANCH_BAD_INPUT;
  }
  if (db == NULL || !db->connected) {
    return BRANCH_DB_UNAVAILABLE;
  }
  const char *params[1] = {id};
  DbResult result;
  memset(&result, 0, sizeof(result));
  if (!db_query(db, "DELETE FROM public.branches WHERE id = $1 RETURNING id", params, 1,
                &result, errbuf, errlen)) {
    BranchStatus status = db_has_sqlstate(errbuf, DB_SQLSTATE_FOREIGN_KEY_VIOLATION)
                              ? BRANCH_CONFLICT
                              : BRANCH_QUERY_FAILED;
    db_result_free(&result);
    return status;
  }
  BranchStatus status = result.nrows == 0 ? BRANCH_NOT_FOUND : BRANCH_OK;
  db_result_free(&result);
  return status;
}
