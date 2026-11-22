/**
 * ============================================================
 *  MOCK SERVICE LAYER  --  THE BACKEND BOUNDARY
 * ============================================================
 *
 * Every function below simulates a network call with a small delay and
 * local/mock data. When the real backend is ready:
 *
 *   1. keep the same function names
 *   2. replace the body with `await fetch('/api/...')`
 *   3. nothing in the UI has to change
 *
 * No real database, email, auth or ISBN service exists yet.
 * ============================================================
 */
import { mockBooks, mockStudents } from '../data/index.js'
import { loginRequestSchema, importRowsSchema, parseWith } from '../validation/schemas.js'
import { API_BASE, ApiError } from './backend.js'

const delay = (ms = 450) => new Promise((resolve) => setTimeout(resolve, ms))

/** Simulated network latency for every mock call. */
export async function simulatedLatency(ms) {
  await delay(ms)
}

/**
 * MOCK AUTH - replace with a real login endpoint later.
 * Works with any email / student ID + password; the chosen role decides
 * which dashboard the user lands on. If the identifier matches a known
 * student, that student's record is used (otherwise the demo student).
 * The payload is validated with zod first - when this becomes a real
 * fetch(), keep the parse in front of the request.
 */
export async function loginRequest(payload) {
  const { email, role } = parseWith(loginRequestSchema, payload)
  await delay()
  if (role === 'admin') {
    return {
      id: 'ad1',
      name: 'Meenakshi Raman',
      email: email.includes('@') ? email : 'librarian@mitsgwalior.in',
      role: 'admin',
      title: 'Chief Librarian',
    }
  }
  const key = String(email).trim().toLowerCase()
  const student =
    mockStudents.find((s) => s.email.toLowerCase() === key || s.studentId.toLowerCase() === key) ||
    mockStudents[0]
  return { ...student, role: 'student' }
}

/** MOCK: would become GET /api/books */
export async function fetchBooks() {
  await delay()
  return mockBooks
}

/** MOCK: would become GET /api/students */
export async function fetchStudents() {
  await delay()
  return mockStudents
}

/** MOCK: would become POST /api/transactions (issue) */
export async function borrowBookRequest() {
  await delay(350)
  return { success: true }
}

/** MOCK: would become POST /api/transactions/:id/return */
export async function returnBookRequest() {
  await delay(350)
  return { success: true }
}

/** MOCK: would become POST /api/reservations */
export async function reserveBookRequest() {
  await delay(350)
  return { success: true }
}

/** Normalises Google's partial dates to the YYYY-MM-DD the date input needs. */
function normalizeDate(value) {
  const date = String(value || '').trim()
  if (/^\d{4}-\d{2}-\d{2}$/.test(date)) return date
  if (/^\d{4}-\d{2}$/.test(date)) return `${date}-01`
  if (/^\d{4}$/.test(date)) return `${date}-01-01`
  return ''
}

function localIsbnLookup(clean) {
  const known = mockBooks.find((b) => b.isbn.replace(/[-\s]/g, '') === clean)
  if (!known) return null
  return {
    isbn: known.isbn,
    found: true,
    title: known.title,
    author: known.author,
    category: known.category,
    publisher: known.publisher,
  }
}

/**
 * ISBN lookup. Calls the backend Google Books proxy, then falls back to the
 * local demo catalog so the UI keeps working when the backend is offline.
 *
 * @param {string} isbn
 * @returns {{ isbn, found, source, title?, author?, category?, publisher?,
 *             publicationDate?, description?, cover? }}
 */
export async function lookupBookByISBN(isbn) {
  const clean = String(isbn).replace(/[-\s]/g, '').toUpperCase()
  let upstreamReached = false

  try {
    const res = await fetch(`${API_BASE}/api/books/lookup/${encodeURIComponent(clean)}`, {
      headers: { Accept: 'application/json' },
    })
    if (res.ok) {
      upstreamReached = true
      const data = await res.json()
      if (data?.found) {
        return {
          isbn: data.isbn || clean,
          found: true,
          source: 'google',
          title: data.title || '',
          author: data.author || '',
          category: data.category || '',
          publisher: data.publisher || '',
          publicationDate: normalizeDate(data.publicationDate),
          description: data.description || '',
          cover: data.cover || '',
        }
      }
    }
  } catch {
    // Backend offline or blocked - use the demo catalog below.
  }

  const local = localIsbnLookup(clean)
  if (local) return { ...local, source: 'local' }
  return { isbn: clean, found: false, source: upstreamReached ? 'google' : 'local' }
}

/**
 * POST /api/books/import - bulk create books
 * @param {Array} rows validated import candidates (see importRowSchema)
 */
export async function importBooksRequest(rows = []) {
  parseWith(importRowsSchema, rows)
  await delay()
  try {
    const data = await fetch(`${API_BASE}/api/books/import`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ books: rows }),
    })
    if (!data.ok) {
      if (data.status === 401) {
        throw new ApiError('Authentication required', { status: 401, code: 'unauthorized' })
      }
      const errData = await data.json().catch(() => ({}))
      throw new Error(errData.message || 'Import failed')
    }
    return await data.json()
  } catch (e) {
    if (e instanceof Error) throw e
    throw new Error('Import failed')
  }
}

/** MOCK: would become GET /api/reports/:type */
export async function generateReportRequest() {
  await delay(700)
  return { success: true }
}
