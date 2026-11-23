/**
 * Translate backend rows (snake_case, numeric strings, SQL dates) into the
 * camelCase shapes the UI already uses. Keeping this in one file means the
 * pages never need to know about database column names.
 */

const text = (value, fallback = '') => (value == null ? fallback : String(value))
const num = (value, fallback = 0) => {
  const parsed = Number(value)
  return Number.isFinite(parsed) ? parsed : fallback
}
const dateOnly = (value) => (value ? String(value).slice(0, 10) : null)

function parseGoogleMetadata(value) {
  if (!value) return {}
  if (typeof value === 'object') return value
  try {
    return JSON.parse(value)
  } catch {
    return {}
  }
}

export function mapBook(row = {}) {
  return {
    id: text(row.id),
    title: text(row.title),
    author: text(row.author),
    isbn: text(row.isbn),
    category: text(row.category),
    publisher: text(row.publisher),
    publicationDate: dateOnly(row.publication_date),
    description: text(row.description),
    cover: text(row.cover),
    totalCopies: num(row.total_copies),
    availableCopies: num(row.available_copies),
    year: num(row.year),
    branchId: row.branch_id || null,
    googleMetadata: parseGoogleMetadata(row.google_metadata),
  }
}

export function mapStudent(row = {}) {
  return {
    id: text(row.id),
    name: text(row.name),
    email: text(row.email),
    studentId: text(row.student_id),
    branch: text(row.branch),
    program: text(row.program),
    semester: num(row.semester, 1),
    contact: text(row.contact),
    joinedDate: dateOnly(row.joined_date),
    status: text(row.status, 'active'),
  }
}

export function mapLoan(row = {}) {
  return {
    id: text(row.id),
    studentId: text(row.student_id),
    bookId: text(row.book_id),
    issueDate: dateOnly(row.issue_date),
    dueDate: dateOnly(row.due_date),
    returnDate: dateOnly(row.return_date),
    fine: num(row.fine),
    status: text(row.status, 'issued'),
  }
}

export function mapReservation(row = {}) {
  return {
    id: text(row.id),
    studentId: text(row.student_id),
    bookId: text(row.book_id),
    reservationDate: dateOnly(row.reservation_date),
    queuePosition: num(row.queue_position, 1),
    status: text(row.status, 'waiting'),
    expectedAvailability: dateOnly(row.expected_availability),
    pickupDeadline: dateOnly(row.pickup_deadline),
  }
}

export function mapFine(row = {}) {
  return {
    id: text(row.id),
    studentId: text(row.student_id),
    bookId: text(row.book_id),
    dueDate: dateOnly(row.due_date),
    returnDate: dateOnly(row.return_date),
    overdueDays: num(row.overdue_days),
    amount: num(row.amount),
    status: text(row.status, 'pending'),
    paidDate: dateOnly(row.paid_date),
  }
}

export function mapNotification(row = {}) {
  return {
    id: text(row.id),
    type: text(row.type, 'announcement'),
    title: text(row.title),
    message: text(row.message),
    time: text(row.time),
    read: Boolean(row.read),
    userId: text(row.user_id),
  }
}

export function mapBranch(row = {}) {
  return { id: text(row.id), name: text(row.name) }
}

export function mapUser(row = {}) {
  return {
    id: text(row.id),
    name: text(row.name),
    email: text(row.email),
    role: text(row.role, 'student'),
    title: text(row.title),
    contact: text(row.contact),
  }
}

/** Book form / import payload -> backend create/update body. */
export function toBookPayload(book = {}) {
  return {
    title: book.title,
    author: book.author || '',
    isbn: book.isbn,
    category: book.category || '',
    publisher: book.publisher || '',
    publication_date: toDbDate(book.publicationDate),
    description: book.description || '',
    cover: book.cover || '',
    total_copies: String(book.totalCopies ?? 1),
    available_copies: String(book.availableCopies ?? book.totalCopies ?? 1),
    year: String(book.year ?? 0),
    branch_id: book.branchId || '',
    google_metadata: JSON.stringify(book.googleMetadata || {}),
  }
}

/** Partial dates from Google Books become the YYYY-MM-DD the API expects. */
function toDbDate(value) {
  const s = String(value ?? '').trim()
  if (/^\d{4}-\d{2}-\d{2}$/.test(s)) return s
  if (/^\d{4}-\d{1,2}$/.test(s)) {
    const [year, month] = s.split('-')
    return `${year}-${month.padStart(2, '0')}-01`
  }
  if (/^\d{4}$/.test(s)) return `${s}-01-01`
  return ''
}

/**
 * A validated bulk-import row -> the exact JSON shape POST /api/books/import
 * reads. The database stores counts as strings and needs a full calendar date.
 */
export function toBookImportPayload(row = {}) {
  const parsed = Number(row.copies)
  const copies = Number.isFinite(parsed) ? Math.min(9999, Math.max(1, Math.floor(parsed))) : 1
  const publicationDate = toDbDate(row.publicationDate)
  const year = /^\d{4}/.test(publicationDate) ? publicationDate.slice(0, 4) : '0'
  return {
    title: row.title || '',
    author: row.author || '',
    isbn: row.isbn || '',
    category: row.category || 'General',
    publisher: row.publisher || '',
    publicationDate,
    description: row.description || '',
    cover: row.cover || '',
    totalCopies: String(copies),
    availableCopies: String(copies),
    year: Number(year) <= 3000 ? year : '0',
    branchId: row.branchId || '',
    googleMetadata: JSON.stringify(row.googleMetadata || {}),
  }
}

/** Student form payload -> backend create/update body. */
export function toStudentPayload(student = {}) {
  const payload = {
    name: student.name,
    email: student.email,
    student_id: student.studentId,
    branch: student.branch || '',
    program: student.program || '',
    semester: String(student.semester ?? 1),
    contact: student.contact || '',
    joined_date: student.joinedDate || '',
    status: student.status || 'active',
  }
  if (student.password) payload.password = student.password
  return payload
}
