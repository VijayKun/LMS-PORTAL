/**
 * Fine records created when a book was returned late.
 * Shape: { id, studentId, bookId, dueDate, returnDate, overdueDays, amount, status }
 * status: 'pending' | 'paid'
 *
 * Note: books that are *currently* overdue are not in this list yet -
 * their fine is calculated live from the transaction (see utils/selectors.js).
 */
export const mockFines = [
  {
    id: 'fn1',
    studentId: 'st1',
    bookId: 'bk12',
    dueDate: '2026-09-01',
    returnDate: '2026-09-03',
    overdueDays: 2,
    amount: 20,
    status: 'paid',
    paidDate: '2026-09-05',
  },
  {
    id: 'fn2',
    studentId: 'st1',
    bookId: 'bk14',
    dueDate: '2026-10-04',
    returnDate: '2026-10-07',
    overdueDays: 3,
    amount: 30,
    status: 'pending',
    paidDate: null,
  },
  {
    id: 'fn3',
    studentId: 'st9',
    bookId: 'bk2',
    dueDate: '2026-09-03',
    returnDate: '2026-09-10',
    overdueDays: 7,
    amount: 70,
    status: 'paid',
    paidDate: '2026-09-12',
  },
  {
    id: 'fn4',
    studentId: 'st13',
    bookId: 'bk28',
    dueDate: '2026-08-31',
    returnDate: '2026-09-02',
    overdueDays: 2,
    amount: 20,
    status: 'paid',
    paidDate: '2026-09-04',
  },
  {
    id: 'fn5',
    studentId: 'st5',
    bookId: 'bk27',
    dueDate: '2026-09-29',
    returnDate: '2026-09-30',
    overdueDays: 1,
    amount: 10,
    status: 'pending',
    paidDate: null,
  },
  {
    id: 'fn6',
    studentId: 'st2',
    bookId: 'bk4',
    dueDate: '2026-08-28',
    returnDate: '2026-09-01',
    overdueDays: 4,
    amount: 40,
    status: 'paid',
    paidDate: '2026-09-03',
  },
  {
    id: 'fn7',
    studentId: 'st15',
    bookId: 'bk8',
    dueDate: '2026-09-20',
    returnDate: '2026-09-25',
    overdueDays: 5,
    amount: 50,
    status: 'pending',
    paidDate: null,
  },
]

export default mockFines
