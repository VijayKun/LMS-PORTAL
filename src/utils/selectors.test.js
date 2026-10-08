import { describe, it, expect } from 'vitest'
import {
  overdueLoans,
  effectiveStatus,
  liveFine,
  studentStats,
  branchStats,
  adminStats,
  outstandingFines,
  totalOf,
} from './selectors.js'
import { addDays, todayISO } from './format.js'
import { FINE_PER_DAY } from '../data/constants.js'

const overdueTx = {
  id: 't-overdue',
  studentId: 's1',
  bookId: 'b1',
  status: 'issued',
  issueDate: addDays(todayISO(), -20),
  dueDate: addDays(todayISO(), -6),
  returnDate: null,
  fine: 0,
}

const dueSoonTx = {
  id: 't-soon',
  studentId: 's1',
  bookId: 'b2',
  status: 'issued',
  issueDate: addDays(todayISO(), -10),
  dueDate: addDays(todayISO(), 3),
  returnDate: null,
  fine: 0,
}

const returnedTx = {
  id: 't-done',
  studentId: 's1',
  bookId: 'b1',
  status: 'returned',
  issueDate: addDays(todayISO(), -40),
  dueDate: addDays(todayISO(), -26),
  returnDate: addDays(todayISO(), -27),
  fine: 0,
}

const state = {
  books: [
    { id: 'b1', title: 'Clean Code', isbn: '9780132350884', totalCopies: 5, branchId: 'cs' },
    { id: 'b2', title: 'Dune', isbn: '9780441013593', totalCopies: 2, branchId: 'ee' },
  ],
  students: [{ id: 's1', name: 'Shivangi Verma', status: 'active' }],
  branches: [
    { id: 'cs', name: 'Computer Science & Engineering' },
    { id: 'ee', name: 'Electrical Engineering' },
  ],
  transactions: [overdueTx, dueSoonTx, returnedTx],
  fines: [],
  reservations: [{ id: 'r1', studentId: 's1', bookId: 'b1', status: 'waiting' }],
  notifications: [],
}

describe('loan status selectors', () => {
  it('flags issued loans past their due date as overdue', () => {
    expect(effectiveStatus(overdueTx)).toBe('overdue')
    expect(effectiveStatus(dueSoonTx)).toBe('issued')
    expect(effectiveStatus(returnedTx)).toBe('returned')
  })

  it('collects only overdue active loans', () => {
    expect(overdueLoans(state).map((t) => t.id)).toEqual(['t-overdue'])
  })

  it('accrues the daily fine on overdue loans only', () => {
    expect(liveFine(overdueTx)).toBe(6 * FINE_PER_DAY)
    expect(liveFine(dueSoonTx)).toBe(0)
  })
})

describe('studentStats', () => {
  it('aggregates a student dashboard', () => {
    const stats = studentStats(state, 's1')
    expect(stats.borrowed).toBe(2)
    expect(stats.dueSoon).toBe(1)
    expect(stats.overdue).toBe(1)
    expect(stats.returned).toBe(1)
    expect(stats.reservations).toBe(1)
    expect(stats.fine).toBe(6 * FINE_PER_DAY)
  })

  it('returns zeroes for an unknown student', () => {
    expect(studentStats(state, 'nobody').borrowed).toBe(0)
  })
})

describe('outstandingFines', () => {
  it('includes live accruals for unreturned overdue loans', () => {
    const fines = outstandingFines(state, 's1')
    expect(fines).toHaveLength(1)
    expect(fines[0].live).toBe(true)
    expect(totalOf(fines)).toBe(6 * FINE_PER_DAY)
  })
})

describe('admin aggregates', () => {
  it('computes branch statistics', () => {
    const stats = branchStats(state)
    const cs = stats.find((b) => b.id === 'cs')
    expect(cs.titles).toBe(1)
    expect(cs.totalCopies).toBe(5)
    expect(cs.transactions).toBe(2)
  })

  it('computes system-wide totals', () => {
    const stats = adminStats(state)
    expect(stats.totalTitles).toBe(2)
    expect(stats.totalCopies).toBe(7)
    expect(stats.issuedCopies).toBe(2)
    expect(stats.overdue).toBe(1)
    expect(stats.students).toBe(1)
  })
})
