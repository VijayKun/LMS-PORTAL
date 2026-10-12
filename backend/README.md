# LMS Backend (C + Mongoose + PostgreSQL/Supabase)

Backend for the college Library Management System. The frontend (React/Vite) is
already deployed on Vercel; this service will provide the REST API.

> **Current status - Milestone 2 (read-only catalog).**
> The server boots and serves `GET /api/health` and a real `GET /api/books`
> that queries Supabase PostgreSQL over `libpq` and returns `{"books":[...]}`.
> When the database is not configured (or the connection fails) the endpoint
> fails safely with `503 database_unavailable`. Authentication, book CRUD,
> loans, reservations and fines are later milestones.

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
│   ├── server/                Mongoose event loop + request dispatch
│   ├── config/                reads env vars (and optional .env) into ServerConfig
│   ├── database/              PostgreSQL boundary: all libpq usage lives here
│   ├── routes/                map method + URL path to a controller
│   ├── controllers/           parse request, call services, build the response
│   ├── services/              library business logic (book catalog now)
│   ├── middleware/            cross-cutting: CORS now, auth later
│   └── utils/                 JSON building, response helpers, validation
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

## API (Milestone 2)

| Method | Path | Status | Meaning |
| --- | --- | --- | --- |
| `GET` | `/api/health` | `200` | server is running |
| `OPTIONS` | any | `204` | CORS preflight |
| `GET` | `/api/books` | `200` | `{"books":[...]}` from `public.books`, ordered by title |
| `GET` | `/api/books` | `503` | database not configured or unreachable |
| `GET` | `/api/books` | `500` | query failed or result too large (generic message) |
| `GET` | `/api/books/:id` | `200` | `{"book":{...}}` for one row of `public.books` |
| `GET` | `/api/books/:id` | `404` | `{"error":"not_found"}` when no such id |
| `GET` | `/api/books/:id` | `503` | database not configured or unreachable |
| `GET` | `/api/students` | `200` | `{"students":[...]}` from `public.students`, ordered by name |
| `GET` | `/api/students` | `503` | database not configured or unreachable |
| `GET` | `/api/students` | `500` | query failed or result too large (generic message) |
| any | unknown `/api/*` | `404` | `{"error":"not_found"}` |

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
  `db/schema.sql` defines the database.
- **Milestone 3** - authentication + authorization middleware (real sessions;
  client-side route guards are not enough), then book CRUD.
- **Milestone 4** - loans, reservations, fines, notifications and reports.

---

## Notes

- The frontend is untouched. It currently uses mock data in `src/services/api.js`;
  switching it to this backend is a separate, deliberate step.
- `libpq` is linked now: the Makefile uses `-IC:/msys64/ucrt64/include` and
  `-LC:/msys64/ucrt64/lib -lpq`. At runtime the libpq/OpenSSL DLLs must be on
  `PATH` (`C:\msys64\ucrt64\bin`).
- The `public.books` column names are defined in exactly one place -
  `BOOKS_SELECT_SQL` in `src/services/book_service.c`. Adjust there if the
  Supabase schema differs.
