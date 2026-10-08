/**
 * Issue / return history (transactions).
 * Shape: { id, studentId, bookId, issueDate, dueDate, returnDate, fine, status }
 * status: 'issued' | 'returned'   ('overdue' is derived from the due date)
 *
 * Dates are 'YYYY-MM-DD' strings so they stay readable and JSON friendly.
 */
export const mockTransactions = [
  // ---- Shivangi Verma (demo student): active loans ----
  { id: 'tx1', studentId: 'st1', bookId: 'bk1', issueDate: '2026-09-28', dueDate: '2026-10-12', returnDate: null, fine: 0, status: 'issued' },
  { id: 'tx2', studentId: 'st1', bookId: 'bk5', issueDate: '2026-09-26', dueDate: '2026-10-10', returnDate: null, fine: 0, status: 'issued' },
  { id: 'tx3', studentId: 'st1', bookId: 'bk7', issueDate: '2026-10-01', dueDate: '2026-10-15', returnDate: null, fine: 0, status: 'issued' },

  // ---- Shivangi Verma: returned books ----
  { id: 'tx4', studentId: 'st1', bookId: 'bk6', issueDate: '2026-09-05', dueDate: '2026-09-19', returnDate: '2026-09-18', fine: 0, status: 'returned' },
  { id: 'tx5', studentId: 'st1', bookId: 'bk12', issueDate: '2026-08-18', dueDate: '2026-09-01', returnDate: '2026-09-03', fine: 20, status: 'returned' },
  { id: 'tx6', studentId: 'st1', bookId: 'bk26', issueDate: '2026-08-02', dueDate: '2026-08-16', returnDate: '2026-08-15', fine: 0, status: 'returned' },
  { id: 'tx7', studentId: 'st1', bookId: 'bk14', issueDate: '2026-09-12', dueDate: '2026-10-04', returnDate: '2026-10-07', fine: 30, status: 'returned' },
  { id: 'tx8', studentId: 'st1', bookId: 'bk16', issueDate: '2026-07-10', dueDate: '2026-07-24', returnDate: '2026-07-23', fine: 0, status: 'returned' },

  // ---- Other students: active loans (some overdue) ----
  { id: 'tx9', studentId: 'st2', bookId: 'bk3', issueDate: '2026-09-20', dueDate: '2026-10-04', returnDate: null, fine: 0, status: 'issued' },
  { id: 'tx10', studentId: 'st3', bookId: 'bk9', issueDate: '2026-09-25', dueDate: '2026-10-09', returnDate: null, fine: 0, status: 'issued' },
  { id: 'tx11', studentId: 'st4', bookId: 'bk22', issueDate: '2026-09-18', dueDate: '2026-10-02', returnDate: null, fine: 0, status: 'issued' },
  { id: 'tx12', studentId: 'st5', bookId: 'bk11', issueDate: '2026-10-02', dueDate: '2026-10-16', returnDate: null, fine: 0, status: 'issued' },
  { id: 'tx13', studentId: 'st6', bookId: 'bk21', issueDate: '2026-09-30', dueDate: '2026-10-14', returnDate: null, fine: 0, status: 'issued' },
  { id: 'tx14', studentId: 'st7', bookId: 'bk15', issueDate: '2026-09-22', dueDate: '2026-10-06', returnDate: null, fine: 0, status: 'issued' },
  { id: 'tx15', studentId: 'st8', bookId: 'bk19', issueDate: '2026-10-03', dueDate: '2026-10-17', returnDate: null, fine: 0, status: 'issued' },
  { id: 'tx16', studentId: 'st10', bookId: 'bk13', issueDate: '2026-09-29', dueDate: '2026-10-13', returnDate: null, fine: 0, status: 'issued' },
  { id: 'tx17', studentId: 'st12', bookId: 'bk17', issueDate: '2026-09-27', dueDate: '2026-10-11', returnDate: null, fine: 0, status: 'issued' },
  { id: 'tx18', studentId: 'st14', bookId: 'bk18', issueDate: '2026-09-24', dueDate: '2026-10-08', returnDate: null, fine: 0, status: 'issued' },
  { id: 'tx19', studentId: 'st16', bookId: 'bk24', issueDate: '2026-10-06', dueDate: '2026-10-20', returnDate: null, fine: 0, status: 'issued' },
  { id: 'tx20', studentId: 'st3', bookId: 'bk10', issueDate: '2026-10-05', dueDate: '2026-10-19', returnDate: null, fine: 0, status: 'issued' },

  // ---- Other students: returned history ----
  { id: 'tx21', studentId: 'st9', bookId: 'bk2', issueDate: '2026-09-14', dueDate: '2026-09-28', returnDate: '2026-09-28', fine: 0, status: 'returned' },
  { id: 'tx22', studentId: 'st9', bookId: 'bk2', issueDate: '2026-08-20', dueDate: '2026-09-03', returnDate: '2026-09-10', fine: 70, status: 'returned' },
  { id: 'tx23', studentId: 'st11', bookId: 'bk25', issueDate: '2026-09-06', dueDate: '2026-09-20', returnDate: '2026-09-19', fine: 0, status: 'returned' },
  { id: 'tx24', studentId: 'st13', bookId: 'bk28', issueDate: '2026-08-17', dueDate: '2026-08-31', returnDate: '2026-09-02', fine: 20, status: 'returned' },
  { id: 'tx25', studentId: 'st15', bookId: 'bk8', issueDate: '2026-09-06', dueDate: '2026-09-20', returnDate: '2026-09-25', fine: 50, status: 'returned' },
  { id: 'tx26', studentId: 'st5', bookId: 'bk27', issueDate: '2026-09-16', dueDate: '2026-09-29', returnDate: '2026-09-30', fine: 10, status: 'returned' },
  { id: 'tx27', studentId: 'st12', bookId: 'bk1', issueDate: '2026-08-25', dueDate: '2026-09-08', returnDate: '2026-09-07', fine: 0, status: 'returned' },
  { id: 'tx28', studentId: 'st2', bookId: 'bk4', issueDate: '2026-08-14', dueDate: '2026-08-28', returnDate: '2026-09-01', fine: 40, status: 'returned' },
  { id: 'tx29', studentId: 'st6', bookId: 'bk23', issueDate: '2026-09-02', dueDate: '2026-09-16', returnDate: '2026-09-15', fine: 0, status: 'returned' },
  { id: 'tx30', studentId: 'st8', bookId: 'bk20', issueDate: '2026-08-24', dueDate: '2026-09-07', returnDate: '2026-09-06', fine: 0, status: 'returned' },
]

export default mockTransactions
