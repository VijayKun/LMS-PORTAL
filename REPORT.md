# LMS — Project Report

Library Management System for **Madhav Institute of Science & Technology, Gwalior**.

- **Stack:** React 19 + Vite 8 (SPA), pure CSS with design tokens (no UI framework), lucide-react icons, recharts charts, zod validation, Vitest + oxlint
- **State:** React Context (auth/theme/toast/library); mock API layer in `src/services/api.js`
- **Deployment:** Vercel (repo: `https://github.com/VijayKun/LMS-PORTAL.git`, branch `main`)
- **Backend:** none yet — every network call is mocked. Replacing a function body with `fetch('/api/...')` is the only change needed later (per the header in `src/services/api.js`).

---

## 1. Application features

**Student area**
- Dashboard (due books, fines, reservations overview)
- Catalog with search/filter, book details, reserving unavailable books
- My Books (issued + borrowing history), Fines (with fine-alert banner), Reservations (queue position, confirm/cancel)
- Student profile & settings that persist edits locally

**Admin area**
- Dashboard (stats, announcements, recent transactions, overdue, popular books)
- Book management: add/edit/delete, details modal, bulk import
- Bulk import: CSV/XLSX upload, header-alias mapping, live per-row validation, step animation, error-report download
- Student management, transactions (issue/return), reservations (mark ready/fulfill/expire), fines
- Reports with CSV export (students, books, transactions, reservations, popular books), analytics charts
- QR scanning, announcements & notifications, full admin settings, data reset

**Cross-cutting**
- Role-based dashboards, themed dark/light mode, toasts, responsive layouts (mobile receives compact record-card layouts on the high-traffic pages, desktop unchanged), keyboard/ARIA affordances

---

## 2. Work done — mobile responsiveness

Desktop UI intentionally unchanged; mobile gets dedicated compact layouts reusing existing components and CSS tokens. Commit `88eb424` (pushed).

- New `.mrec-*` (mobile record card) CSS family + `.info-list.compact`, `.fines-stats`, `.fine-alert`, `.filter-bar` rules in `src/index.css`
- Pages migrated to dual rendering — desktop table (`.hide-mobile`) + mobile card list (`.only-mobile`):
  - `src/pages/student/StudentFines.jsx`
  - `src/pages/student/BorrowingHistory.jsx` (search placeholder shortened)
  - `src/pages/student/StudentReservations.jsx`
  - `src/pages/admin/AdminFines.jsx`
- Verified: lint clean, 46/46 unit tests, production build, pushed.

---

## 3. Work done — frontend security hardening

Commit `09d265a` (pushed). Each item references the original security review finding.

### 3.1 Privilege escalation via `updateUser` (Finding 1)
`src/store/AuthContext.jsx` previously spread arbitrary patch keys, so a caller could set `role`/`id`. Now only `name`, `email`, `contact`, `avatar` are accepted (`UPDATABLE_FIELDS`); verified callers (admin `Settings.jsx`) only use those.

### 3.2 Forgeable client session — hardened client-side, backend still required (Finding 2)
Role is still forgeable by editing `sessionStorage['lms.auth.v1']` — this is inherent to a mock frontend. Improved what the client controls:
- `readSavedUser` now rejects sessions that lack `id`/`name`/valid `role` (shape validation) and removes corrupted storage.
- Legacy `localStorage` session key `lms.auth.v1.local` is purged on logout and cannot be revived (Finding 5).

### 3.3 Route guards are UX only (Finding 3)
Client `<RequireAuth>` guards remain best-effort. Real authorization must be enforced by the backend; the report's backend checklist (section 5) captures this.

### 3.4 Security headers (Finding 4)
`vercel.json` now ships (applies to all routes on Vercel; local dev unaffected):
- **Content-Security-Policy:** `default-src 'self'`; `script-src 'self'`; `style-src 'self' 'unsafe-inline'` (React inline styles); `img-src 'self' data: blob: https:`; `font-src 'self'`; `connect-src 'self'`; `object-src 'none'`; `base-uri 'self'`; `frame-ancestors 'none'`; `form-action 'self'`; `upgrade-insecure-requests`
- `X-Content-Type-Options: nosniff` · `X-Frame-Options: DENY` · `Referrer-Policy: strict-origin-when-cross-origin` · HSTS (`max-age=63072000; includeSubDomains`) · `Permissions-Policy` (camera/mic/geolocation denied)

Two dependencies of the CSP were fixed so `script-src 'self'` is possible:
- The inline theme bootstrap in `index.html` was extracted to `public/theme-init.js`.
- Google Fonts CDN `<link>`s removed (see 3.5) so no third-party origin is needed.

### 3.5 Self-hosted fonts (Finding 6)
Replaced the Google Fonts CDN with `@fontsource/inter` (400/500/600/700) + `@fontsource/dancing-script` (600/700), imported in `src/main.jsx`. No network request to a third party at runtime; fonts are served from the app's own origin (satisfies `font-src 'self'`).

### 3.6 Login hardening (Finding 7 — frontend part)
- Previously present: Caps-Lock warning, validation messages, demo-mode notes.
- Backend still owns the real rules: bcrypt/argon2 hashing, minimum length + breach check, rate limiting / lockout (section 5).

### 3.7 CSV injection + bulk-import validation (Finding 8)
- `src/utils/csv.js` → `escapeCell` now prefixes cells starting with `= + - @ tab CR` with `'` so exported reports can't become spreadsheet formulas; cells that parse as numbers (`-5`) are left intact. Covered by new unit tests.
- `src/utils/importRows.js` hardening: field length caps (title 300, author 200, category 100, publisher 200) with new `too-long` fail status + reason; `copies` clamped to 1–9999.
- Already present before this pass and kept: 5 MB file-size cap in `BulkImport.jsx`, `accept=".csv,.xlsx"`, ISBN-10/13 checksum validation, duplicate detection, unknown-column tolerance (unknown headers are never imported).

### 3.8 Zod schemas at the API boundary (Finding 9)
New `src/validation/schemas.js`: `loginRequestSchema`, `importRowSchema`, `importRowsSchema`, and `parseWith` helper. Wired into `src/services/api.js`:
- `loginRequest` validates `{ email, role }` before the "call".
- `importBooksRequest(rows)` validates each candidate row; `BulkImport.jsx` now passes the ready rows.
When the mock layer is swapped for real `fetch()`, these parsers sit in front of every request.

### 3.9 Supply chain + error leakage (Finding 10)
- `npm run audit` script added (`npm audit --audit-level=moderate`). Current install: **0 vulnerabilities**.
- `.github/dependabot.yml`: weekly updates for npm (grouped prod/dev) and GitHub Actions.
- `ErrorBoundary.jsx`: error message/stack now render only under `import.meta.env.DEV`; production shows a generic message (details still hit `console.error`).

### 3.10 Findings 2/3/7/11 backend responsibilities — documented, not implementable in frontend
See section 5.

---

## 4. Verification

| Command | Result |
| --- | --- |
| `npm run lint` | Clean (only pre-existing warnings, none from new code) |
| `npm run test` | **50/50** pass (includes new CSV-injection and import-length-cap tests) |
| `npm run build` | Production build OK; fonts bundled locally |
| `npm run audit` | 0 vulnerabilities |

All work committed and pushed to `origin/main` (`09d265a`), deployed via Vercel.

---

## 5. Backend checklist (recommended for the real API)

The following cannot be enforced client-side and must live on the server:

1. **Authorization on every endpoint** — never trust the React role; derive it from the authenticated session server-side. Route guards in the SPA are UX only (Finding 3).
2. **Server-issued sessions** — HttpOnly, Secure, SameSite cookies (or short-lived signed tokens). Never store roles in `localStorage`/`sessionStorage` where they're forgeable (Finding 2).
3. **Real authentication** — bcrypt/argon2 password hashing + per-user salt, minimum-length and NIST "known-password" checks, login rate limiting and lockout, password reset flow (Finding 7).
4. **Bulk import revalidation** — re-validate every imported row server-side (title/isbn/copies/field caps). Client checks are convenience, not security (Finding 8).
5. **CSV formula sanitisation** — also on the server for any report the backend generates (Finding 8).
6. **PII minimisation** — do not return contact/address fields the caller doesn't need; paginate and redact lists (Finding 11).
7. **Audit logging** — log admin mutations (book import, student edits, fine adjustments) with actor + timestamp for forensics (Finding 11).
8. **Session expiry & revocation** — absolute + idle timeouts, and a server-side kill switch so a logged-out user's token can't be replayed (Finding 11).