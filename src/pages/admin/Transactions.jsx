import { useEffect, useMemo, useRef, useState } from 'react'
import { Plus, Eye, RotateCcw, ArrowLeftRight, SearchX, ScanLine, X } from 'lucide-react'
import { useLibrary } from '../../store/LibraryContext.jsx'
import { usePageTitle } from '../../store/PageTitleContext.jsx'
import { useToast } from '../../store/ToastContext.jsx'
import SearchBar from '../../components/ui/SearchBar.jsx'
import { Select, Field } from '../../components/ui/Field.jsx'
import Badge from '../../components/ui/Badge.jsx'
import BookCover from '../../components/books/BookCover.jsx'
import EmptyState from '../../components/ui/EmptyState.jsx'
import Modal from '../../components/ui/Modal.jsx'
import ConfirmDialog from '../../components/ui/ConfirmDialog.jsx'
import CopyText from '../../components/ui/CopyText.jsx'
import {
  getStudent,
  getBook,
  effectiveStatus,
  availableCopiesOf,
  liveFine,
} from '../../utils/selectors.js'
import { formatDate, addDays, todayISO } from '../../utils/format.js'
import { LOAN_PERIOD_DAYS, MAX_BOOKS_PER_STUDENT } from '../../data/constants.js'
import { borrowBookRequest, returnBookRequest } from '../../services/api.js'

const TABS = [
  { id: 'all', label: 'All' },
  { id: 'issued', label: 'Issued' },
  { id: 'returned', label: 'Returned' },
  { id: 'overdue', label: 'Overdue' },
]

export default function Transactions() {
  usePageTitle('Transactions')
  const { state, borrowBook, returnBook, track } = useLibrary()
  const { toast } = useToast()

  const [tab, setTab] = useState('all')
  const [query, setQuery] = useState('')
  const [issuing, setIssuing] = useState(false)
  const [issueForm, setIssueForm] = useState({ studentId: '', bookId: '' })
  const [returning, setReturning] = useState(null)
  const [viewing, setViewing] = useState(null)
  const [scan, setScan] = useState('')
  const scanRef = useRef(null)

  // Counter convenience: the scan box is ready for the first code on load.
  useEffect(() => {
    if (window.matchMedia('(min-width: 769px)').matches) scanRef.current?.focus()
  }, [])

  /** Enter a student ID / email / ISBN: fills the issue form step by step. */
  function handleScan(e) {
    if (e.key !== 'Enter') return
    e.preventDefault()
    const code = scan.trim().toLowerCase()
    if (!code) return

    const cleanIsbn = code.replace(/-/g, '')
    const student =
      state.students.find(
        (s) =>
          s.studentId.toLowerCase() === code ||
          s.email?.toLowerCase() === code ||
          s.id === code,
      ) || null
    const book =
      state.books.find((b) => b.isbn.replace(/-/g, '') === cleanIsbn || b.id === code) || null

    if (student) {
      setIssueForm((f) => ({ ...f, studentId: student.id }))
      toast(`${student.name} captured — now scan the book barcode.`, 'info')
    } else if (book) {
      if (availableCopiesOf(state, book) === 0) {
        toast(`"${book.title}" has no free copies right now.`, 'error')
      } else {
        setIssueForm((f) => ({ ...f, bookId: book.id }))
        toast(`"${book.title}" captured.`, 'info')
      }
    } else {
      toast('No student or book matched that code.', 'error')
    }
    setScan('')
  }

  const scanStudent = getStudent(state, issueForm.studentId)
  const scanBook = getBook(state, issueForm.bookId)

  const dueDate = addDays(todayISO(), LOAN_PERIOD_DAYS)

  const rows = useMemo(() => {
    const q = query.trim().toLowerCase()
    return state.transactions
      .filter((tx) => {
        const student = getStudent(state, tx.studentId)
        const book = getBook(state, tx.bookId)
        const status = effectiveStatus(tx)
        if (tab === 'issued' && tx.status !== 'issued') return false
        if (tab === 'returned' && tx.status !== 'returned') return false
        if (tab === 'overdue' && status !== 'overdue') return false
        if (q && !`${student?.name} ${student?.studentId} ${book?.title}`.toLowerCase().includes(q)) return false
        return true
      })
      .sort((a, b) => {
        const da = a.returnDate || a.issueDate
        const db = b.returnDate || b.issueDate
        return db < da ? -1 : 1
      })
  }, [state, tab, query])

  const counts = {
    all: state.transactions.length,
    issued: state.transactions.filter((t) => t.status === 'issued').length,
    returned: state.transactions.filter((t) => t.status === 'returned').length,
    overdue: state.transactions.filter((t) => effectiveStatus(t) === 'overdue').length,
  }

  const availableBooks = state.books.filter((b) => availableCopiesOf(state, b) > 0)

  function issueBook(e) {
    e.preventDefault()
    const student = getStudent(state, issueForm.studentId)
    const book = getBook(state, issueForm.bookId)
    if (!student || !book) {
      toast('Select both a student and a book.', 'error')
      return
    }
    if (student.status !== 'active') {
      toast('This student account is suspended.', 'error')
      return
    }
    borrowBook(student.id, book.id) // optimistic
    track(borrowBookRequest())
    setIssuing(false)
    setIssueForm({ studentId: '', bookId: '' })
    toast(`"${book.title}" issued to ${student.name} until ${formatDate(dueDate)}.`, 'success')
  }

  function confirmReturn() {
    const tx = returning
    returnBook(tx.id) // optimistic
    track(returnBookRequest())
    const book = getBook(state, tx.bookId)
    setReturning(null)
    const fine = liveFine(tx)
    toast(
      fine > 0
        ? `"${book.title}" returned with a ₹${fine} fine.`
        : `"${book.title}" marked as returned successfully.`,
      fine > 0 ? 'info' : 'success',
    )
  }

  return (
    <>
      <div className="page-header">
        <div>
          <h1>Transactions</h1>
          <p className="page-sub">Issue and return books, and follow up on overdue loans.</p>
        </div>
        <div className="page-actions">
          <button className="btn btn-primary" onClick={() => setIssuing(true)}>
            <Plus size={16} /> Issue Book
          </button>
        </div>
      </div>

      {/* -------- Quick issue / scan box -------- */}
      <div className="card card-pad mb-4 quick-issue">
        <div className="quick-issue-row">
          <span className="quick-issue-icon" aria-hidden="true">
            <ScanLine size={18} />
          </span>
          <input
            ref={scanRef}
            className="quick-issue-input"
            value={scan}
            onChange={(e) => setScan(e.target.value)}
            onKeyDown={handleScan}
            placeholder="Scan or type Student ID / ISBN, then press Enter…"
            aria-label="Quick issue: scan student ID or book ISBN"
          />
          {scanStudent && (
            <span className="scan-chip">
              {scanStudent.name}
              <button
                type="button"
                onClick={() => setIssueForm((f) => ({ ...f, studentId: '' }))}
                aria-label="Clear scanned student"
              >
                <X size={12} />
              </button>
            </span>
          )}
          {scanBook && (
            <span className="scan-chip">
              {scanBook.title}
              <button
                type="button"
                onClick={() => setIssueForm((f) => ({ ...f, bookId: '' }))}
                aria-label="Clear scanned book"
              >
                <X size={12} />
              </button>
            </span>
          )}
          <button
            className="btn btn-primary"
            disabled={!scanStudent || !scanBook}
            onClick={() => setIssuing(true)}
          >
            Issue →
          </button>
        </div>
        <p className="text-xs text-muted mt-2">
          Two-step counter flow: scan the library card first, then the book barcode — or type
          either code by hand.
        </p>
      </div>

      <div className="tabs">
        {TABS.map((t) => (
          <button key={t.id} className={`tab ${tab === t.id ? 'active' : ''}`} onClick={() => setTab(t.id)}>
            {t.label}<span className="count">{counts[t.id]}</span>
          </button>
        ))}
      </div>

      <div className="card card-pad mb-4">
        <SearchBar value={query} onChange={setQuery} placeholder="Search by student, ID or book title..." />
      </div>

      <div className="card">
        <div className="card-body flush">
          {rows.length === 0 ? (
            <EmptyState
              icon={SearchX}
              title="No transactions found"
              message="No records match this tab and search."
            />
          ) : (
            <div className="table-responsive">
              <table className="table">
                <thead>
                  <tr>
                    <th>Student</th>
                    <th>Book</th>
                    <th>Issue Date</th>
                    <th>Due Date</th>
                    <th>Return Date</th>
                    <th>Fine</th>
                    <th>Status</th>
                    <th className="text-right">Action</th>
                  </tr>
                </thead>
                <tbody>
                  {rows.map((tx) => {
                    const student = getStudent(state, tx.studentId)
                    const book = getBook(state, tx.bookId)
                    const status = effectiveStatus(tx)
                    return (
                      <tr key={tx.id}>
                        <td>
                          <div className="book-title">{student?.name}</div>
                          <div className="book-meta">{student?.studentId}</div>
                        </td>
                        <td>
                          <div className="table-cell-book">
                            <div className="cover-sm-row"><BookCover book={book} size="sm" /></div>
                            <div>
                              <div className="book-title">{book?.title}</div>
                              <div className="book-meta">{book?.author}</div>
                            </div>
                          </div>
                        </td>
                        <td>{formatDate(tx.issueDate)}</td>
                        <td className="strong">{formatDate(tx.dueDate)}</td>
                        <td>{tx.returnDate ? formatDate(tx.returnDate) : '—'}</td>
                        <td>
                          {status === 'overdue' ? (
                            <span className="text-danger font-bold">₹{liveFine(tx)}</span>
                          ) : tx.fine > 0 ? (
                            <span className="text-danger font-semibold">₹{tx.fine}</span>
                          ) : (
                            '—'
                          )}
                        </td>
                        <td>
                          <Badge
                            status={status === 'returned' ? 'returned' : status === 'overdue' ? 'overdue' : 'issued'}
                          >
                            {status === 'returned' ? 'Returned' : status === 'overdue' ? 'Overdue' : 'Issued'}
                          </Badge>
                        </td>
                        <td>
                          <div className="actions-cell">
                            <button className="icon-btn" data-tip="View transaction" onClick={() => setViewing(tx)} aria-label="View transaction">
                              <Eye size={15} />
                            </button>
                            {tx.status === 'issued' && (
                              <button className="icon-btn" data-tip="Mark as returned" onClick={() => setReturning(tx)} aria-label="Mark as returned">
                                <RotateCcw size={15} />
                              </button>
                            )}
                          </div>
                        </td>
                      </tr>
                    )
                  })}
                </tbody>
              </table>
            </div>
          )}
        </div>
      </div>

      {/* -------- Issue book -------- */}
      <Modal
        open={issuing}
        title="Issue Book"
        onClose={() => setIssuing(false)}
        footer={
          <>
            <button className="btn btn-secondary" onClick={() => setIssuing(false)}>Cancel</button>
            <button className="btn btn-primary" form="issue-form" type="submit">Issue Book</button>
          </>
        }
      >
        <form id="issue-form" onSubmit={issueBook}>
          <p className="text-sm text-secondary mb-3">
            The book will be issued until <strong>{formatDate(dueDate)}</strong> ({LOAN_PERIOD_DAYS} days).
            Each student may hold up to {MAX_BOOKS_PER_STUDENT} books.
          </p>
          <Field label="Student" required>
            <Select value={issueForm.studentId} onChange={(e) => setIssueForm({ ...issueForm, studentId: e.target.value })}>
              <option value="">Select a student...</option>
              {state.students
                .filter((s) => s.status === 'active')
                .map((s) => (
                  <option key={s.id} value={s.id}>
                    {s.name} ({s.studentId})
                  </option>
                ))}
            </Select>
          </Field>
          <Field label="Book" required hint={availableBooks.length === 0 ? 'No copies are available right now.' : 'Only books with a free copy are listed.'}>
            <Select value={issueForm.bookId} onChange={(e) => setIssueForm({ ...issueForm, bookId: e.target.value })}>
              <option value="">Select a book...</option>
              {availableBooks.map((b) => (
                <option key={b.id} value={b.id}>
                  {b.title} — {availableCopiesOf(state, b)} available
                </option>
              ))}
            </Select>
          </Field>
        </form>
      </Modal>

      {/* -------- Transaction details -------- */}
      <Modal open={Boolean(viewing)} title="Transaction Details" onClose={() => setViewing(null)}
        footer={<button className="btn btn-secondary" onClick={() => setViewing(null)}>Close</button>}
      >
        {viewing && (
          <div className="info-list">
            <div className="info-row"><span className="info-label">Transaction ID</span><span className="info-value"><CopyText value={viewing.id.toUpperCase()} label="transaction ID" /></span></div>
            <div className="info-row"><span className="info-label">Student</span><span className="info-value">{getStudent(state, viewing.studentId)?.name}</span></div>
            <div className="info-row"><span className="info-label">Book</span><span className="info-value">{getBook(state, viewing.bookId)?.title}</span></div>
            <div className="info-row"><span className="info-label">Issue date</span><span className="info-value">{formatDate(viewing.issueDate)}</span></div>
            <div className="info-row"><span className="info-label">Due date</span><span className="info-value">{formatDate(viewing.dueDate)}</span></div>
            <div className="info-row"><span className="info-label">Return date</span><span className="info-value">{viewing.returnDate ? formatDate(viewing.returnDate) : 'Not returned yet'}</span></div>
            <div className="info-row"><span className="info-label">Fine</span><span className="info-value">₹{viewing.fine || (effectiveStatus(viewing) === 'overdue' ? liveFine(viewing) : 0)}</span></div>
            <div className="info-row"><span className="info-label">Status</span><span className="info-value"><Badge status={effectiveStatus(viewing)}>{effectiveStatus(viewing)}</Badge></span></div>
          </div>
        )}
      </Modal>

      <ConfirmDialog
        open={Boolean(returning)}
        title="Mark as returned?"
        danger={false}
        confirmLabel="Mark Returned"
        message={
          returning
            ? `"${getBook(state, returning.bookId)?.title}" will be closed and the copy marked available.`
            : ''
        }
        onClose={() => setReturning(null)}
        onConfirm={confirmReturn}
      />
    </>
  )
}
