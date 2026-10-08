# LibManage — College Library Management System

A complete, responsive frontend for a campus library: students browse and borrow books, librarians manage the catalog, loans, fines and analytics. Runs entirely in the browser with a mock service layer that is ready to be swapped for a real backend.

## Quick start

```bash
npm install
npm run dev       # http://localhost:5173
```

Other scripts:

| Script | Purpose |
| --- | --- |
| `npm run dev` | Start the Vite dev server |
| `npm run build` | Production build into `dist/` |
| `npm run preview` | Serve the production build |
| `npm run lint` | Lint with oxlint |
| `npm test` | Run the Vitest suite once |
| `npm run test:watch` | Run tests in watch mode |

## Demo login

Open the app and pick a role on the login screen — credentials are prefilled.

- **Student** — any email or student ID (e.g. `2025CS1042`) with any password
- **Admin / Librarian** — `librarian@college.edu` with any password

> Demo mode: there is no backend yet, so passwords are not verified and all data lives in `localStorage`.

## Features

**Student panel**
- Dashboard with loans, due-soon reminders, notifications and new arrivals
- Catalog with search, filters, book details, borrowing and reservations
- My books (renew/return), reservation queue, borrowing history
- Fine tracking and profile editing

**Admin panel**
- Dashboard with key metrics and branch activity
- Book CRUD, bulk import, student CRUD and per-student detail tabs
- Issue/return counter, reservation approvals, fine collection
- Branch management, notifications, analytics charts, reports and settings

**Cross-cutting**
- Light/dark theme, toasts, confirm dialogs, pagination, empty/loading states
- Fully responsive (mobile drawer navigation, stacking layouts, scrollable tables)
- Print-friendly report view and real CSV import/export

## Bulk import

`/admin/import` parses real files in the browser:

- `.csv` — parsed by a built-in RFC 4180 parser (`src/utils/csv.js`)
- `.xlsx` — parsed with [read-excel-file](https://github.com/catamphetamine/read-excel-file)
- Required columns: **Book Name**, **ISBN** — optional: **Copies**, **Branch**, **Author**, **Category**, **Publisher**
- Header names are matched loosely (`Title`, `Qty`, `Location`, …)
- Every row is validated (ISBN checksum, duplicates against the catalog, empty titles) and valid rows are written straight into the catalog
- Failed/skipped rows can be downloaded as a real CSV error report

## Reports

`/admin/reports` generates five report types with date/branch/status filters. **Export CSV** downloads the generated table; **Print** opens a print-optimised view (navigation and filters hidden).

## Architecture

```
src/
├── components/
│   ├── layout/        # Sidebar, Topbar, DashboardLayout, role layouts
│   └── ui/            # Modal, Badge, Field, Pagination, StatCard, ...
├── pages/
│   ├── student/       # 9 student routes
│   ├── admin/         # 14 admin routes
│   ├── Login.jsx
│   └── NotFound.jsx
├── store/             # Contexts: Library (state), Auth, Toast, Theme, PageTitle
├── services/api.js    # MOCK SERVICE LAYER — the backend boundary
├── data/              # Seed dataset (~1,100 lines) loaded into localStorage
└── utils/             # csv, importRows, format, selectors (all pure/tested)
```

- **State** — `LibraryContext` is a `useReducer` store persisted to `localStorage['lms.data.v1']`. Every feature action (borrow, return, reserve, addBook, payFine, ...) lives there. Use **Settings → Reset demo data** to restore the seed dataset.
- **Backend boundary** — `services/api.js` simulates network latency for login, ISBN lookup, import and reports. To connect a real backend, replace each function body with `fetch('/api/...')`; the UI does not change.
- **Styling** — one hand-written design system in `src/index.css` (CSS custom properties, light + dark themes, print styles, responsive breakpoints at 1280/992/768/480 px).

## Testing

Vitest + Testing Library (jsdom):

- `src/utils/csv.test.js` — CSV parser/generator round-trips
- `src/utils/importRows.test.js` — header mapping, ISBN checksums, duplicate detection
- `src/utils/format.test.js` — date/number formatting helpers
- `src/utils/selectors.test.js` — derived stats (overdue, fines, branch/admin aggregates)
- `src/store/LibraryContext.test.jsx` — reducer flows: add, borrow/return, reserve, reset

## Roadmap (needs a backend)

- Real authentication (sessions/tokens, password reset by email)
- ISBN enrichment against a live book metadata API
- Online fine payments, CSV export for very large report sets
- Email/SMS due-date reminders
