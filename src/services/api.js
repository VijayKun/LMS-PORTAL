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

/**
 * MOCK ISBN LOOKUP - the bulk import page calls this while "fetching book
 * information". Later it can be replaced with a real ISBN/db/books API so
 * titles, authors and covers are filled in automatically.
 *
 * @param {string} isbn
 * @returns {{ isbn, found, title?, author?, category?, publisher? }}
 */
export async function lookupBookByISBN(isbn) {
  await delay(500)
  const clean = String(isbn).replace(/-/g, '')
  const known = mockBooks.find((b) => b.isbn.replace(/-/g, '') === clean)
  if (known) {
    return {
      isbn: known.isbn,
      found: true,
      title: known.title,
      author: known.author,
      category: known.category,
      publisher: known.publisher,
    }
  }
  return { isbn, found: false }
}

/**
 * MOCK: would become POST /api/books/import
 * @param {Array} rows validated import candidates (see importRowSchema)
 */
export async function importBooksRequest(rows = []) {
  parseWith(importRowsSchema, rows)
  await delay(600)
  return { success: true }
}

/** MOCK: would become GET /api/reports/:type */
export async function generateReportRequest() {
  await delay(700)
  return { success: true }
}
