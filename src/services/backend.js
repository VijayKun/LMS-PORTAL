/**
 * Real backend client.
 *
 * In development the browser talks to the Vite dev server and Vite proxies
 * `/api` to the C backend (see vite.config.js), so the session cookie is
 * same-origin. In production set VITE_API_URL to the deployed backend origin.
 */
import {
  mapBook,
  mapBranch,
  mapFine,
  mapLoan,
  mapNotification,
  mapReservation,
  mapStudent,
  mapUser,
  toBookPayload,
  toStudentPayload,
} from './mappers.js'

export const API_BASE = (import.meta.env.VITE_API_URL || '').replace(/\/+$/, '')

export class ApiError extends Error {
  constructor(message, { status = 0, code = 'error' } = {}) {
    super(message)
    this.name = 'ApiError'
    this.status = status
    this.code = code
  }
}

/** True when the API could not be reached (backend down / CORS failure). */
export function isOfflineError(error) {
  return (
    !error ||
    error.code === 'network_error' ||
    error.status === 0 ||
    error.status === 502 ||
    error.status === 503
  )
}

async function request(path, { method = 'GET', body, signal } = {}) {
  const headers = { Accept: 'application/json' }
  const init = { method, credentials: 'include', headers, signal }
  if (body !== undefined) {
    headers['Content-Type'] = 'application/json'
    init.body = JSON.stringify(body)
  }

  let res
  try {
    res = await fetch(`${API_BASE}${path}`, init)
  } catch {
    throw new ApiError('Cannot reach the library server.', { code: 'network_error' })
  }

  const raw = await res.text()
  let data = null
  if (raw) {
    try {
      data = JSON.parse(raw)
    } catch {
      data = null
    }
  }

  if (!res.ok) {
    throw new ApiError(data?.message || `Request failed (${res.status}).`, {
      status: res.status,
      code: data?.error || 'http_error',
    })
  }
  return data
}

/* ---------------------------- authentication ---------------------------- */

export async function login(identifier, password) {
  const data = await request('/api/auth/login', {
    method: 'POST',
    body: { identifier, password },
  })
  return data?.user ? mapUser(data.user) : null
}

export async function logout() {
  try {
    await request('/api/auth/logout', { method: 'POST' })
  } catch {
    /* logging out locally still works when the server is unreachable */
  }
}

export async function me() {
  const data = await request('/api/auth/me')
  return data?.user ? mapUser(data.user) : null
}

export async function health() {
  return request('/api/health')
}

/* ------------------------------- read side ------------------------------- */

async function listAll(path, key, { limit = 100, maxPages = 20 } = {}) {
  const items = []
  for (let page = 0; page < maxPages; page++) {
    const separator = path.includes('?') ? '&' : '?'
    const data = await request(`${path}${separator}limit=${limit}&offset=${items.length}`)
    const rows = Array.isArray(data?.[key]) ? data[key] : []
    items.push(...rows)
    if (rows.length < limit) break
  }
  return items
}

export async function fetchBooks() {
  return (await listAll('/api/books', 'books')).map(mapBook)
}

export async function fetchBranches() {
  return (await listAll('/api/branches', 'branches')).map(mapBranch)
}

export async function fetchStudents() {
  return (await listAll('/api/students', 'students')).map(mapStudent)
}

export async function fetchStudent(id) {
  const data = await request(`/api/students/${encodeURIComponent(id)}`)
  return data?.student ? mapStudent(data.student) : null
}

export async function fetchLoans() {
  return (await listAll('/api/transactions', 'loans')).map(mapLoan)
}

export async function fetchReservations() {
  return (await listAll('/api/reservations', 'reservations')).map(mapReservation)
}

export async function fetchFines() {
  return (await listAll('/api/fines', 'fines')).map(mapFine)
}

export async function fetchNotifications() {
  return (await listAll('/api/notifications', 'notifications')).map(mapNotification)
}

/**
 * Everything the LibraryProvider needs for one signed-in user. Admins get the
 * full directory; students get the catalog plus their own activity.
 */
export async function fetchWorkspace(user) {
  const [books, branches] = await Promise.all([fetchBooks(), fetchBranches()])

  if (user.role === 'admin') {
    const [students, transactions, reservations, fines, notifications] = await Promise.all([
      fetchStudents(),
      fetchLoans(),
      fetchReservations(),
      fetchFines(),
      fetchNotifications(),
    ])
    return { books, branches, students, transactions, reservations, fines, notifications }
  }

  const [self, transactions, reservations, fines, notifications] = await Promise.all([
    fetchStudent(user.id).catch(() => null),
    fetchLoans(),
    fetchReservations(),
    fetchFines(),
    fetchNotifications(),
  ])

  return {
    books,
    branches,
    students: self ? [self] : [],
    transactions,
    reservations,
    fines,
    notifications,
  }
}

/* ---------------------------- student actions ---------------------------- */

export async function issueLoan(studentId, bookId, dueDate = '') {
  return request('/api/transactions', {
    method: 'POST',
    body: { student_id: studentId, book_id: bookId, due_date: dueDate },
  })
}

export async function returnLoan(loanId) {
  return request(`/api/transactions/${encodeURIComponent(loanId)}/return`, { method: 'POST' })
}

export async function createReservation(studentId, bookId) {
  return request('/api/reservations', {
    method: 'POST',
    body: { student_id: studentId, book_id: bookId },
  })
}

export async function cancelReservation(reservationId) {
  return request(`/api/reservations/${encodeURIComponent(reservationId)}`, {
    method: 'DELETE',
  })
}

export async function markReservationReady(reservationId) {
  return request(`/api/reservations/${encodeURIComponent(reservationId)}/ready`, {
    method: 'POST',
  })
}

export async function setReservationStatus(reservationId, status) {
  return request(`/api/reservations/${encodeURIComponent(reservationId)}/status`, {
    method: 'POST',
    body: { status },
  })
}

export async function payFine(fineId) {
  return request(`/api/fines/${encodeURIComponent(fineId)}/pay`, { method: 'POST' })
}

/* --------------------------- notification actions ------------------------ */

export async function createNotification({ type, title, message, userId = 'all' }) {
  return request('/api/notifications', {
    method: 'POST',
    body: { type, title, message, user_id: userId },
  })
}

export async function markNotificationRead(id) {
  return request(`/api/notifications/${encodeURIComponent(id)}/read`, { method: 'POST' })
}

export async function markAllNotificationsRead() {
  return request('/api/notifications/read-all', { method: 'POST' })
}

/* ------------------------------ book actions ----------------------------- */

export async function createBook(book) {
  return request('/api/books', { method: 'POST', body: toBookPayload(book) })
}

export async function updateBook(book) {
  return request(`/api/books/${encodeURIComponent(book.id)}`, {
    method: 'PUT',
    body: toBookPayload(book),
  })
}

export async function deleteBook(id) {
  return request(`/api/books/${encodeURIComponent(id)}`, { method: 'DELETE' })
}

/* ----------------------------- student actions --------------------------- */

export async function createStudent(student) {
  return request('/api/students', { method: 'POST', body: toStudentPayload(student) })
}

export async function updateStudent(student) {
  return request(`/api/students/${encodeURIComponent(student.id)}`, {
    method: 'PUT',
    body: toStudentPayload(student),
  })
}

export async function deleteStudent(id) {
  return request(`/api/students/${encodeURIComponent(id)}`, { method: 'DELETE' })
}

/* ------------------------------ branch actions --------------------------- */

export async function createBranch(branch) {
  return request('/api/branches', {
    method: 'POST',
    body: { id: branch.id || '', name: branch.name },
  })
}

export async function updateBranch(branch) {
  return request(`/api/branches/${encodeURIComponent(branch.id)}`, {
    method: 'PUT',
    body: { name: branch.name },
  })
}

export async function deleteBranch(id) {
  return request(`/api/branches/${encodeURIComponent(id)}`, { method: 'DELETE' })
}

/* -------------------------------- reports -------------------------------- */

export async function fetchReport(type) {
  return request(`/api/reports/${encodeURIComponent(type)}`)
}
