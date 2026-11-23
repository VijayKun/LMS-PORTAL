-- LMS backend - Supabase PostgreSQL schema.
--
-- Apply with a superuser/database password connection, e.g.:
--   psql "host=db.<ref>.supabase.co port=5432 dbname=postgres user=postgres password=<pw> sslmode=require" -f backend/db/schema.sql
--
-- Idempotent: safe to run more than once. It never drops or alters existing
-- objects, so it will not destroy live data.
--
-- Column names for the catalog mirror BOOKS_SELECT_SQL in
-- src/services/book_service.c - keep the two in sync.

BEGIN;

-- ---- Academic branches / departments ------------------------------------

CREATE TABLE IF NOT EXISTS branches (
  id   text PRIMARY KEY,
  name text NOT NULL
);

-- ---- Book catalog --------------------------------------------------------

CREATE TABLE IF NOT EXISTS books (
  id               text PRIMARY KEY,
  title            text NOT NULL,
  author           text NOT NULL DEFAULT '',
  isbn             text NOT NULL UNIQUE,
  category         text NOT NULL DEFAULT '',
  publisher        text NOT NULL DEFAULT '',
  publication_date date,
  description      text NOT NULL DEFAULT '',
  cover            text NOT NULL DEFAULT '',
  total_copies     integer NOT NULL DEFAULT 1 CHECK (total_copies >= 0),
  available_copies integer NOT NULL DEFAULT 1 CHECK (available_copies >= 0),
  year             smallint NOT NULL DEFAULT 0,
  branch_id        text REFERENCES branches (id) ON DELETE SET NULL,
  google_metadata  jsonb NOT NULL DEFAULT '{}'::jsonb,
  created_at       timestamptz NOT NULL DEFAULT now(),
  updated_at       timestamptz NOT NULL DEFAULT now(),
  CHECK (available_copies <= total_copies)
);

CREATE INDEX IF NOT EXISTS idx_books_title    ON books (title);
CREATE INDEX IF NOT EXISTS idx_books_category ON books (category);
CREATE INDEX IF NOT EXISTS idx_books_branch   ON books (branch_id);

-- ---- Students ------------------------------------------------------------

CREATE TABLE IF NOT EXISTS students (
  id          text PRIMARY KEY,
  name        text NOT NULL,
  email       text NOT NULL UNIQUE,
  student_id  text NOT NULL UNIQUE,
  branch      text NOT NULL DEFAULT '',
  program     text NOT NULL DEFAULT '',
  semester    smallint NOT NULL DEFAULT 1 CHECK (semester >= 1),
  contact     text NOT NULL DEFAULT '',
  joined_date date,
  status      text NOT NULL DEFAULT 'active' CHECK (status IN ('active', 'suspended')),
  created_at  timestamptz NOT NULL DEFAULT now()
);

-- Existing databases created before student auth need the new column too.
ALTER TABLE public.students ADD COLUMN IF NOT EXISTS password_hash text;

-- ---- Staff (library admins) - password_hash is filled in by Milestone 3 ----

CREATE TABLE IF NOT EXISTS admins (
  id            text PRIMARY KEY,
  name          text NOT NULL,
  email         text NOT NULL UNIQUE,
  role          text NOT NULL DEFAULT 'admin' CHECK (role IN ('admin', 'librarian')),
  title         text NOT NULL DEFAULT '',
  contact       text NOT NULL DEFAULT '',
  password_hash text,
  created_at    timestamptz NOT NULL DEFAULT now()
);

-- ---- Issue / return transactions ----------------------------------------

CREATE TABLE IF NOT EXISTS transactions (
  id          text PRIMARY KEY,
  student_id  text NOT NULL REFERENCES students (id) ON DELETE CASCADE,
  book_id     text NOT NULL REFERENCES books (id) ON DELETE RESTRICT,
  issue_date  date NOT NULL,
  due_date    date NOT NULL,
  return_date date,
  fine        numeric(10, 2) NOT NULL DEFAULT 0 CHECK (fine >= 0),
  status      text NOT NULL DEFAULT 'issued' CHECK (status IN ('issued', 'returned')),
  created_at  timestamptz NOT NULL DEFAULT now()
);

CREATE INDEX IF NOT EXISTS idx_tx_student    ON transactions (student_id);
CREATE INDEX IF NOT EXISTS idx_tx_book       ON transactions (book_id);
CREATE INDEX IF NOT EXISTS idx_tx_status_due ON transactions (status, due_date);

-- ---- Reservations --------------------------------------------------------

CREATE TABLE IF NOT EXISTS reservations (
  id                    text PRIMARY KEY,
  student_id            text NOT NULL REFERENCES students (id) ON DELETE CASCADE,
  book_id               text NOT NULL REFERENCES books (id) ON DELETE CASCADE,
  reservation_date      date NOT NULL DEFAULT CURRENT_DATE,
  queue_position        integer NOT NULL DEFAULT 1 CHECK (queue_position >= 1),
  status                text NOT NULL DEFAULT 'waiting'
                        CHECK (status IN ('waiting', 'ready', 'fulfilled', 'expired')),
  expected_availability date,
  pickup_deadline       date,
  created_at            timestamptz NOT NULL DEFAULT now()
);

CREATE INDEX IF NOT EXISTS idx_res_student     ON reservations (student_id);
CREATE INDEX IF NOT EXISTS idx_res_book_status ON reservations (book_id, status);

-- ---- Fines ---------------------------------------------------------------

CREATE TABLE IF NOT EXISTS fines (
  id           text PRIMARY KEY,
  student_id   text NOT NULL REFERENCES students (id) ON DELETE CASCADE,
  book_id      text NOT NULL REFERENCES books (id) ON DELETE RESTRICT,
  due_date     date NOT NULL,
  return_date  date,
  overdue_days integer NOT NULL DEFAULT 0 CHECK (overdue_days >= 0),
  amount       numeric(10, 2) NOT NULL DEFAULT 0 CHECK (amount >= 0),
  status       text NOT NULL DEFAULT 'pending' CHECK (status IN ('pending', 'paid')),
  paid_date    date,
  created_at   timestamptz NOT NULL DEFAULT now()
);

CREATE INDEX IF NOT EXISTS idx_fines_student ON fines (student_id);
CREATE INDEX IF NOT EXISTS idx_fines_status  ON fines (status);

-- ---- Notifications & announcements --------------------------------------
-- user_id is a student id, an admin id, or the literal 'all'.

CREATE TABLE IF NOT EXISTS notifications (
  id      text PRIMARY KEY,
  type    text NOT NULL
          CHECK (type IN ('due', 'overdue', 'fine', 'reservation', 'announcement')),
  title   text NOT NULL,
  message text NOT NULL DEFAULT '',
  time    timestamptz NOT NULL DEFAULT now(),
  read    boolean NOT NULL DEFAULT false,
  user_id text NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_notif_user ON notifications (user_id, time DESC);

COMMIT;
