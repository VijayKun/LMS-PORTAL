/**
 * Selectors = functions that derive view data from the store.
 * Keeping them here stops the same calculations being repeated in every page.
 */
import { daysRemaining, daysBetween, todayISO, addDays } from './format.js'
import { FINE_PER_DAY, LOAN_PERIOD_DAYS } from '../data/constants.js'

export const getBook = (state, bookId) => state.books.find((b) => b.id === bookId)
export const getStudent = (state, studentId) => state.students.find((s) => s.id === studentId)
export const getBranch = (state, branchId) => state.branches.find((b) => b.id === branchId)

export const branchName = (state, branchId) => getBranch(state, branchId)?.name || '—'

/** 'issued' loans whose due date has passed are shown as 'overdue'. */
export function effectiveStatus(tx) {
  if (tx.status === 'issued' && daysRemaining(tx.dueDate) < 0) return 'overdue'
  return tx.status
}

/** Fine accrued so far on an active (unreturned) loan. */
export function liveFine(tx) {
  const overdueBy = -daysRemaining(tx.dueDate)
  return overdueBy > 0 ? overdueBy * FINE_PER_DAY : 0
}

export const isOverdue = (tx) => effectiveStatus(tx) === 'overdue'
export const isActiveLoan = (tx) => tx.status === 'issued'

export const activeLoansOf = (state, studentId) =>
  state.transactions.filter((t) => t.studentId === studentId && isActiveLoan(t))

export const overdueLoans = (state) =>
  state.transactions.filter((t) => isActiveLoan(t) && isOverdue(t))

/** Due within N days (inclusive) and not yet returned. */
export const dueWithin = (state, studentId, days) =>
  activeLoansOf(state, studentId).filter((t) => {
    const left = daysRemaining(t.dueDate)
    return left >= 0 && left <= days
  })

/** Fines still to be paid: settled records + live overdue accruals. */
export function outstandingFines(state, studentId) {
  const settled = state.fines.filter(
    (f) => f.studentId === studentId && f.status === 'pending',
  )
  const live = activeLoansOf(state, studentId)
    .filter(isOverdue)
    .map((t) => ({
      id: `live-${t.id}`,
      studentId,
      bookId: t.bookId,
      dueDate: t.dueDate,
      returnDate: null,
      overdueDays: -daysRemaining(t.dueDate),
      amount: liveFine(t),
      status: 'pending',
      live: true,
    }))
  return [...settled, ...live]
}

export const totalOf = (list) => list.reduce((sum, item) => sum + item.amount, 0)

export const reservationsOf = (state, studentId) =>
  state.reservations.filter((r) => r.studentId === studentId)

export const notificationsOf = (state, userId) =>
  state.notifications
    .filter((n) => n.userId === userId || n.userId === 'all')
    .sort((a, b) => (a.time < b.time ? 1 : -1))

/** Live availability - recomputed whenever copies are issued or returned. */
export const availableCopiesOf = (state, book) => {
  const issuedNow = state.transactions.filter(
    (t) => t.bookId === book.id && isActiveLoan(t),
  ).length
  return Math.max(0, book.totalCopies - issuedNow)
}

/** ---------------- Admin dashboard aggregates ---------------- */

export function adminStats(state) {
  const active = state.transactions.filter(isActiveLoan)
  const totalCopies = state.books.reduce((sum, b) => sum + b.totalCopies, 0)
  const issuedCopies = active.length
  return {
    totalTitles: state.books.length,
    totalCopies,
    availableCopies: totalCopies - issuedCopies,
    issuedCopies,
    students: state.students.filter((s) => s.status === 'active').length,
    overdue: overdueLoans(state).length,
    pendingFines: totalOf(state.fines.filter((f) => f.status === 'pending')),
    collectedFines: totalOf(state.fines.filter((f) => f.status === 'paid')),
    activeReservations: state.reservations.filter(
      (r) => r.status === 'waiting' || r.status === 'ready',
    ).length,
  }
}

export function branchStats(state) {
  return state.branches.map((branch) => {
    const books = state.books.filter((b) => b.branchId === branch.id)
    const bookIds = books.map((b) => b.id)
    const txs = state.transactions.filter((t) => bookIds.includes(t.bookId))
    const active = txs.filter(isActiveLoan)
    const totalCopies = books.reduce((sum, b) => sum + b.totalCopies, 0)
    const issuedCopies = active.length
    const studentsWithBorrow = new Set(txs.map((t) => t.studentId))
    return {
      ...branch,
      titles: books.length,
      totalCopies,
      issuedCopies,
      availableCopies: totalCopies - issuedCopies,
      transactions: txs.length,
      students: studentsWithBorrow.size,
    }
  })
}

/** ---------------- Student dashboard aggregates ---------------- */

export function studentStats(state, studentId) {
  const active = activeLoansOf(state, studentId)
  const soon = active.filter((t) => {
    const left = daysRemaining(t.dueDate)
    return left >= 0 && left <= 7
  })
  const reservations = reservationsOf(state, studentId).filter(
    (r) => r.status === 'waiting' || r.status === 'ready',
  )
  const fines = outstandingFines(state, studentId)
  return {
    borrowed: active.length,
    dueSoon: soon.length,
    reservations: reservations.length,
    fine: totalOf(fines),
    overdue: active.filter(isOverdue).length,
    returned: state.transactions.filter(
      (t) => t.studentId === studentId && t.status === 'returned',
    ).length,
  }
}

/** Convenience for the "days remaining" pill on due-soon cards. */
export function dueLabel(tx) {
  const left = daysRemaining(tx.dueDate)
  if (left < 0) return `${Math.abs(left)} day${Math.abs(left) === 1 ? '' : 's'} overdue`
  if (left === 0) return 'Due today'
  return `${left} day${left === 1 ? '' : 's'} remaining`
}

/**
 * Issues per month for a student (or everyone when studentId is null),
 * oldest first - used for the StatCard sparklines.
 */
export function borrowSeries(state, studentId = null, months = 6) {
  const now = new Date()
  const counts = []
  for (let i = months - 1; i >= 0; i--) {
    const d = new Date(now.getFullYear(), now.getMonth() - i, 1)
    const key = `${d.getFullYear()}-${String(d.getMonth() + 1).padStart(2, '0')}`
    counts.push(
      state.transactions.filter(
        (t) =>
          t.issueDate.startsWith(key) && (!studentId || t.studentId === studentId),
      ).length,
    )
  }
  return counts
}

/** Percentage change between the last two points of a series. */
export function seriesTrend(series, upIsGood = true) {
  if (!Array.isArray(series) || series.length < 2) return null
  const prev = series[series.length - 2]
  const next = series[series.length - 1]
  const pct = Math.round((Math.abs(next - prev) / Math.max(1, prev)) * 100)
  const dir = next >= prev ? 'up' : 'down'
  const good = dir === 'up' ? upIsGood : !upIsGood
  return { pct, dir, good }
}

export { daysRemaining, daysBetween, todayISO, addDays, LOAN_PERIOD_DAYS, FINE_PER_DAY }
