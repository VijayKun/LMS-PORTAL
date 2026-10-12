# LMS Backend (C + Mongoose + PostgreSQL/Supabase)

Backend for the college Library Management System. The frontend (React/Vite) is
already deployed on Vercel; this service will provide the REST API.

> **Current status - Milestone 3 (authentication + read-only catalog).**
> The server boots and serves `GET /api/health`, the real catalog (books /
> students / loans / reservations / fines / notifications) over Supabase
> PostgreSQL (`libpq`), and **login/logout/current-user with server-side
> sessions** (`scrypt` password hashing + HttpOnly session cookies with
> absolute and idle timeouts). When the database is not configured (or the
> connection fails) every database endpoint fails safely with
> `503 database_unavailable`. Book/student CRUD, loan reservations and fines
> computation are later milestones.

---

## Tech stack

| Layer | Choice |
| --- | --- |
| Language | C (C11) |
| HTTP server | [Mongoose](https://github.com/cesanta/mongoose) 7.23 (single-file, vendored in `lib/`) |
| Database | Supabase PostgreSQL |
| DB driver | PostgreSQL `libpq` (integrated in Milestone 2) |
| Data format | JSON build / JSON errors (small hand-written helpers) |

---

## Prerequisites

All tools come from **MSYS2 UCRT64** on Windows.

```bash
# Open the "MSYS2 UCRT64" terminal (or add C:\msys64\ucrt64\bin and
# C:\msys64\usr\bin to your PATH) and run:

pacman -S --needed make
pacman -S --needed mingw-w64-ucrt-x86_64-postgresql   # libpq, needed from Milestone 2

gcc --version     # GCC (UCRT64)
make --version    # GNU Make
```

Mongoose is already vendored in `lib/` - nothing to install. See `lib/README.md`.

---

## Directory structure

```
backend/
├── src/
│   ├── main.c                 entry point: load config, init db handle, start server
│   ├── server/                Mongoose event loop + request dispatch (AppContext owns SessionStore)
│   ├── config/                reads env vars (and optional .env) into ServerConfig
│   ├── database/              PostgreSQL boundary: all libpq usage lives here
│   ├── routes/                map method + URL path to a controller
│   ├── controllers/           parse request, call services, build the response
│   ├── services/              library business logic (catalog, auth, lists)
│   ├── auth/                  server-side session store (tokens + TTLs)
│   ├── security/              password hashing (scrypt via OpenSSL)
│   ├── middleware/            cross-cutting: CORS, authentication (`auth_require`)
│   └── utils/                 JSON building/parsing, response helpers, validation
├── lib/                       vendored third-party code (mongoose.c/.h)
├── db/                        SQL schema for Supabase (schema.sql)
├── include/                   reserved for installed/public headers (currently unused)
├── tests/                     unit tests (plain C, run with `make test`)
├── build/                     compiler output (git-ignored, created by build)
├── .env.example               template for environment variables
├── .gitignore
├── Makefile
└── README.md
```

**Folder purposes in plain terms**

- **routes vs controllers vs services** - a request flows *route → controller →
  service*. The route knows *which URL*; the controller knows *how to read the
  request and answer it*; the service holds the *real business rules* (issue a
  book, compute a fine). Keeping them apart means each file stays small.
- **config** - never hardcode secrets; everything comes from environment
  variables (or a local `.env` that is git-ignored).
- **database** - the only place that talks to PostgreSQL via `libpq`.
- **middleware** - things that happen around every request (CORS headers, and
  later authentication/authorization).
- **utils** - reusable helpers so we don't repeat JSON/response logic.

> **Reserved (not created yet):** `include/` and the
> `user_*` / `loan_*` / `reservation_*` / `fine_*` / `notification_*` route,
> controller and service files are added as each feature is implemented. They
> are intentionally not stubbed to keep the code honest and readable.

---

## Configuration

Copy the example file and fill in your values. On Windows PowerShell:

```powershell
Set-Location backend
Copy-Item .env.example .env
# then edit .env and fill in the Supabase values
notepad .env
```

Instead of a `.env` file you can set real environment variables for the
current session (they override `.env`):

```powershell
$env:LMS_DB_HOST     = "db.your-project-ref.supabase.co"
$env:LMS_DB_PORT     = "5432"
$env:LMS_DB_NAME     = "postgres"
$env:LMS_DB_USER     = "postgres.your-project-ref"
$env:LMS_DB_PASSWORD = "your-database-password"
$env:LMS_DB_SSLMODE  = "require"
```

| Variable | Purpose | Default |
| --- | --- | --- |
| `LMS_SERVER_HOST` | interface to bind | `127.0.0.1` |
| `LMS_SERVER_PORT` | TCP port | `8080` |
| `LMS_CORS_ALLOWED_ORIGIN` | origin allowed to call the API | `http://localhost:5173` |
| `LMS_DB_HOST` | Supabase database host | *(empty)* |
| `LMS_DB_PORT` | database port | `5432` |
| `LMS_DB_NAME` | database name | `postgres` |
| `LMS_DB_USER` | database user | *(empty)* |
| `LMS_DB_PASSWORD` | database password | *(empty)* |
| `LMS_DB_SSLMODE` | TLS mode (Supabase requires `require`) | `require` |
| `LMS_SESSION_TTL_SECONDS` | absolute session lifetime | `43200` (12 h) |
| `LMS_SESSION_TTL_IDLE_SECONDS` | idle session timeout (`0` disables) | `1800` (30 min) |
| `LMS_COOKIE_SECURE` | set the `Secure` flag on the session cookie (production only) | `0` |

Real environment variables override `.env`. The DB password is never printed at
startup (shown as `(set)` / `(empty)`). **Never commit `.env`.**

---

## Database schema

`db/schema.sql` defines the PostgreSQL (Supabase) tables: `branches`, `books`,
`students`, `admins`, `transactions`, `reservations`, `fines` and
`notifications`. It is idempotent (`CREATE TABLE IF NOT EXISTS`, no drops), so
it is safe to re-run. Apply it once against the Supabase database:

```bash
psql "host=db.<ref>.supabase.co port=5432 dbname=postgres user=postgres password=<pw> sslmode=require" \
  -f db/schema.sql
```

The catalog column names in `books` mirror `BOOKS_SELECT_SQL` in
`src/services/book_service.c` - keep the two in sync.

---

## Build and run

Make sure `C:\msys64\ucrt64\bin` (toolchain, `libpq`, runtime DLLs) and
`C:\msys64\usr\bin` (`make`) are on your `PATH`. From PowerShell:

```powershell
# one-time, for this terminal session
$env:Path = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;" + $env:Path

Set-Location backend
make            # compile -> build/lms_backend.exe
make test       # compile and run the unit tests
make run        # build (if needed) and run the server
make clean      # delete build/
make help       # list targets
```

### Try it

With the server running (`make run`) open a second PowerShell window:

```powershell
curl.exe http://127.0.0.1:8080/api/health
# {"status":"ok","service":"lms-backend","version":"0.1.0","uptime_seconds":3}

# With valid Supabase credentials in .env / the environment:
curl.exe http://127.0.0.1:8080/api/books
# {"books":[{"id":...,"title":"...","author":"...", ...}, ...]}

# Without a database (or on connection failure):
# HTTP 503 {"error":"database_unavailable","message":"The book catalog is temporarily unavailable."}
```

Press **Ctrl + C** to stop the server.

---

## API (Milestone 3)

### Authentication and sessions

| Method | Path | Status | Meaning |
| --- | --- | --- | --- |
| `POST` | `/api/auth/login` | `200` | `{"admin":{...},"expires_in":N}`; sets `lms_session` (HttpOnly, `SameSite=Lax`, `Max-Age=ttl`) |
| `POST` | `/api/auth/login` | `400` | missing `email`/`password`, body too large, or malformed body |
| `POST` | `/api/auth/login` | `401` | unknown email or wrong password (same generic message) |
| `POST` | `/api/auth/login` | `503` | database not configured or unreachable |
| `POST` | `/api/auth/logout` | `204` | destroys the session and clears the cookie |
| `GET` | `/api/auth/me` | `200` | `{"admin":{...},"expires_in":N}` for the current session |
| `GET` | `/api/auth/me` | `401` | no/invalid/expired session |
| any | `/api/auth/*` | `405` | wrong method for that path |

Sessions are server-side (a `SessionStore` in the `AppContext`; up to 1024
entries). A login sends the token as an HttpOnly cookie (`lms_session`) and also
accepts `Authorization: Bearer <token>` for non-browser clients. There is no
client-side role trust: the controller reads the role from the stored session.
An idle session is dropped after `LMS_SESSION_TTL_IDLE_SECONDS`; an absolute
cap applies via `LMS_SESSION_TTL_SECONDS`.

Passwords are hashed with OpenSSL **scrypt** (`EVP_PBE_scrypt`, `N=16384 r=8
p=1`, random 16-byte salt) and stored in `public.admins.password_hash` as
`scrypt$N$r$p$salt_hex$key_hex`. To seed an admin password once:

```powershell
.\build\lms_backend.exe hash "your-password"
# -> scrypt$16384$8$1$<salt_hex>$<key_hex>   (paste, plus an UPDATE statement, into the DB)
```

> **Note:** `login` will `503` until the admin row has a non-empty
> `password_hash`.

### Catalog (all read-only, open)

| Method | Path | Status | Meaning |
| --- | --- | --- | --- |
| `GET` | `/api/health` | `200` | server is running |
| `OPTIONS` | any | `204` | CORS preflight |
| `GET` | `/api/books` | `200` | `{"books":[...]}` from `public.books`, ordered by title |
| `GET` | `/api/books` | `503` | database not configured or unreachable |
| `GET` | `/api/books` | `500` | query failed or result too large (generic message) |
| `GET` | `/api/books?q=&category=&limit=&offset=` | `200` | filtered + paginated catalog; response includes `limit`/`offset` |
| `GET` | `/api/books/:id` | `200` | `{"book":{...}}` for one row of `public.books` |
| `GET` | `/api/books/:id` | `404` | `{"error":"not_found"}` when no such id |
| `GET` | `/api/books/:id` | `503` | database not configured or unreachable |
| `GET` | `/api/books/isbn/:isbn` | `200` | `{"book":{...}}` by unique ISBN |
| `GET` | `/api/books/isbn/:isbn` | `404` | `{"error":"not_found"}` when no such ISBN |
| `POST` | `/api/books` | `201` | `{"book":{...}}`; requires a valid session (JSON body) |
| `POST` | `/api/books` | `400` | `invalid_body`: missing title/isbn, bad numbers, bad date |
| `POST` | `/api/books` | `401` | no/invalid/expired session |
| `POST` | `/api/books` | `409` | `conflict`: the ISBN already exists |
| `POST` | `/api/books` | `415` | `unsupported_media_type`: body is not `application/json` |
| `POST` | `/api/books` | `503` | database not configured or unreachable |
| `PUT` | `/api/books/:id` | `200` | `{"book":{...}}`; updates only the provided fields (session required) |
| `PUT` | `/api/books/:id` | `400/401/404/409/503` | same error contract as `POST` |
| `DELETE` | `/api/books/:id` | `200` | `{"ok":true}`; requires a valid session |
| `DELETE` | `/api/books/:id` | `404` | `{"error":"not_found"}` when no such id |
| `DELETE` | `/api/books/:id` | `409` | `conflict`: referenced by loans (FK `ON DELETE RESTRICT`), cannot be deleted |
| `GET` | `/api/students` | `200` | `{"students":[...]}` from `public.students`, ordered by name |
| `GET` | `/api/students` | `503` | database not configured or unreachable |
| `GET` | `/api/students` | `500` | query failed or result too large (generic message) |
| `GET` | `/api/students?q=&branch=&limit=&offset=` | `200` | filtered + paginated student list; response includes `limit`/`offset` |
| `GET` | `/api/students/:id` | `200` | `{"student":{...}}` for one row of `public.students` |
| `GET` | `/api/students/:id` | `404` | `{"error":"not_found"}` when no such id |
| `GET` | `/api/students/:id` | `503` | database not configured or unreachable |
| `POST` | `/api/students` | `201` | `{"student":{...}}`; requires a valid session (JSON body) |
| `POST` | `/api/students` | `400` | `invalid_body`: name/email/student_id missing or bad values |
| `POST` | `/api/students` | `401` | no/invalid/expired session |
| `POST` | `/api/students` | `409` | `conflict`: the email or student_id already exists |
| `POST` | `/api/students` | `415` | `unsupported_media_type`: body is not `application/json` |
| `POST` | `/api/students` | `503` | database not configured or unreachable |
| `PUT` | `/api/students/:id` | `200` | `{"student":{...}}`; updates only the provided fields (session required) |
| `PUT` | `/api/students/:id` | `400/401/404/409/503` | same error contract as `POST` |
| `DELETE` | `/api/students/:id` | `200` | `{"ok":true}`; requires a valid session |
| `DELETE` | `/api/students/:id` | `404` | `{"error":"not_found"}` when no such id |
| `GET` | `/api/loans` | `200` | `{"loans":[...]}` with `student_id`/`book_id`/`status` filters, `limit`/`offset` |
| `GET` | `/api/loans/:id` | `200/404/503` | `{"loan":{...}}` for one row |
| `GET` | `/api/reservations` | `200` | `{"reservations":[...]}` with `student_id`/`book_id`/`status` filters |
| `GET` | `/api/reservations/:id` | `200/404/503` | `{"reservation":{...}}` for one row |
| `GET` | `/api/fines` | `200` | `{"fines":[...]}` with `student_id`/`status` filters |
| `GET` | `/api/fines/:id` | `200/404/503` | `{"fine":{...}}` for one row |
| `GET` | `/api/notifications` | `200` | `{"notifications":[...]}` with `user_id`/`type` filters |
| `GET` | `/api/notifications/:id` | `200/404/503` | `{"notification":{...}}` for one row |
| `POST` | `/api/transactions` | `201` | `{"loan":{...}}`; issue a book (JSON body: `student_id`, `book_id`, optional `due_date`) - session required |
| `POST` | `/api/transactions` | `400/401/404/409/415/503` | invalid body, no session, student/book missing, no copy available |
| `POST` | `/api/transactions/:id/return` | `200` | `{"loan":{...}}`; return a book; creates a pending fine when overdue - session required |
| `POST` | `/api/transactions/:id/return` | `401/404/503` | no session or no issued loan with that id |
| `POST` | `/api/reservations` | `201` | `{"reservation":{...}}`; place a reservation (JSON body: `student_id`, `book_id`) - session required |
| `POST` | `/api/reservations` | `400/401/404/409/415/503` | duplicate active reservation, student/book missing |
| `POST` | `/api/reservations/:id/ready` | `200` | `{"reservation":{...}}`; mark waiting reservation ready - session required |
| `POST` | `/api/reservations/:id/ready` | `401/404/503` | no waiting reservation |
| `DELETE` | `/api/reservations/:id` | `200` | `{"ok":true}`; cancel waiting/ready reservation - session required |
| `DELETE` | `/api/reservations/:id` | `401/404/503` | no active reservation |
| `POST` | `/api/fines/:id/pay` | `200` | `{"ok":true}`; mark pending fine as paid - session required |
| `POST` | `/api/fines/:id/pay` | `401/404/503` | no pending fine with that id |
| `GET` | `/api/reports/:type` | `200` | `{"report":{...}}` for the requested report (e.g. `overview`) |
| `GET` | `/api/reports/:type` | `404/503` | unknown report type or DB unavailable |
| any | unknown `/api/*` | `404` | `{"error":"not_found"}` |

All list endpoints share the same pagination contract: filters are `$`-prefixed
columns, `limit` (default `100`, max `500`) and `offset` clamp the result, and
the response echoes `{"<key>":[...],"limit":L,"offset":O}`. Loans, fines and
notifications are currently **read-only** rows - they become writable in
Milestone 4.

The book write endpoints reuse the same `BookMutate` body
(`title, author, isbn, category, publisher, publication_date, description,
cover, branch_id, total_copies, available_copies, year`): an absent field is
skipped on `PUT` and falls back to the schema default on `POST`. `title` and
`isbn` are required when creating; numbers are validated (`available_copies <=
total_copies`, years 0-3000, real `YYYY-MM-DD` dates) and oversized values get
`400` before the database is touched. New ids come from
`gen_random_uuid()::text` (PostgreSQL 13+, which Supabase provides).

Student writes work the same way with `StudentMutate`
(`name, email, student_id, branch, program, semester, contact, joined_date,
status`): `name`, `email` and `student_id` are required on create, `semester`
must be `>= 1`, `status` must be `active` or `suspended`, and `joined_date`
must be a real date. Deleting a student cascades to their transactions,
reservations and fines (schema `ON DELETE CASCADE`).

The `/api/books` handler selects only `id, title, author, isbn, category,
publisher, total_copies, available_copies` from `public.books`, ordered by
`title`. SQL `NULL` becomes JSON `null`; values are escaped by the shared JSON
helpers. Internal database errors are logged to the server console only and are
never returned to clients.

---

## Roadmap

- **Milestone 2 (done)** - `database/db.c` connects to Supabase with `libpq`
  (`PQconnectdbParams` + `PQexecParams`); `book_service_list()` implements
  `GET /api/books` and `book_service_get()` implements `GET /api/books/:id` as
  read-only JSON; `student_service_list()` implements `GET /api/students`.
  `db/schema.sql` defines the database. Catalog search (`q`/`category`/
  `limit`/`offset`), `GET /api/books/isbn/:isbn` and `GET /api/students/:id` are
  also done.
- **Milestone 3 (in progress)** - authentication is done (`security/password.c`,
  `auth/session_store.c`, `auth_service`, `auth_middleware`, `/api/auth/*`).
  Book and student CRUD are done (`book_service_*` / `student_service_*`,
  `POST/PUT/DELETE` routes protected by `auth_require`, ISBN / email /
  student_id conflicts as `409`, loan-reference and cascade rules honoured).
  Remaining: loan/reservation/fine/notification write flows (Milestone 4).

---

## Notes

- The frontend is untouched. It currently uses mock data in `src/services/api.js`;
  switching it to this backend is a separate, deliberate step.
- Linking now: `-IC:/msys64/ucrt64/include` / `-LC:/msys64/ucrt64/lib`
  `-lpq -lcrypto -lws2_32`. At runtime the libpq/OpenSSL/ws2_32 DLLs must be on
  `PATH` (`C:\msys64\ucrt64\bin`).
- The `public.books` column names are defined in exactly one place -
  `BOOKS_SELECT_SQL` in `src/services/book_service.c`. Adjust there if the
  Supabase schema differs.
- The `admins` table needs a `password_hash` before login works; seed it with
  `.\build\lms_backend.exe hash "<password>"`. Remember `LMS_COOKIE_SECURE=1`
  in production.
