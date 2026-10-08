import { useState } from 'react'
import { Link, useNavigate } from 'react-router-dom'
import { BookOpen, RotateCcw } from 'lucide-react'
import { useLibrary } from '../../store/LibraryContext.jsx'
import { useAuth } from '../../store/AuthContext.jsx'
import { usePageTitle } from '../../store/PageTitleContext.jsx'
import { useToast } from '../../store/ToastContext.jsx'
import Badge from '../../components/ui/Badge.jsx'
import BookCover from '../../components/books/BookCover.jsx'
import EmptyState from '../../components/ui/EmptyState.jsx'
import ConfirmDialog from '../../components/ui/ConfirmDialog.jsx'
import LoanTimeline from '../../components/ui/LoanTimeline.jsx'
import SwipeableRow from '../../components/ui/SwipeableRow.jsx'
import {
  activeLoansOf,
  getBook,
  effectiveStatus,
  dueLabel,
  branchName,
  daysRemaining,
} from '../../utils/selectors.js'
import { formatDate, todayISO } from '../../utils/format.js'
import { returnBookRequest } from '../../services/api.js'

const TABS = [
  { id: 'borrowed', label: 'Currently Borrowed' },
  { id: 'due', label: 'Due Soon' },
  { id: 'returned', label: 'Returned' },
]

export default function MyBooks() {
  usePageTitle('My Books')
  const { state, returnBook, track } = useLibrary()
  const { user } = useAuth()
  const { toast } = useToast()
  const navigate = useNavigate()

  const [tab, setTab] = useState('borrowed')
  const [returning, setReturning] = useState(null)

  const active = activeLoansOf(state, user.id)
  const dueSoon = active.filter((t) => daysRemaining(t.dueDate) <= 7).sort((a, b) => (a.dueDate < b.dueDate ? -1 : 1))
  const returned = state.transactions
    .filter((t) => t.studentId === user.id && t.status === 'returned')
    .sort((a, b) => (b.returnDate < a.returnDate ? -1 : 1))

  const rows = tab === 'borrowed' ? active : tab === 'due' ? dueSoon : returned
  const counts = { borrowed: active.length, due: dueSoon.length, returned: returned.length }

  function confirmReturn() {
    const tx = returning
    returnBook(tx.id) // optimistic
    track(returnBookRequest())
    setReturning(null)
    const book = getBook(state, tx.bookId)
    const left = daysRemaining(tx.dueDate)
    if (left < 0) {
      toast(`"${book.title}" returned. A fine was recorded for ${Math.abs(left)} late day(s).`, 'info')
    } else {
      toast(`"${book.title}" returned successfully.`, 'success')
    }
  }

  return (
    <>
      <div className="page-header">
        <div>
          <h1>My Books</h1>
          <p className="page-sub">Track everything you have borrowed from the library.</p>
        </div>
        <div className="page-actions">
          <Link className="btn btn-secondary" to="/student/books">
            Browse more books
          </Link>
        </div>
      </div>

      <div className="tabs">
        {TABS.map((t) => (
          <button
            key={t.id}
            className={`tab ${tab === t.id ? 'active' : ''}`}
            onClick={() => setTab(t.id)}
          >
            {t.label}
            <span className="count">{counts[t.id]}</span>
          </button>
        ))}
      </div>

      {tab !== 'returned' && (
        <LoanTimeline
          items={rows.map((tx) => ({
            id: tx.id,
            title: getBook(state, tx.bookId)?.title || 'Book',
            issueDate: tx.issueDate,
            dueDate: tx.dueDate,
          }))}
        />
      )}

      <div className="card">
        <div className="card-body flush">
          {rows.length === 0 ? (
            <EmptyState
              kind={tab === 'returned' ? 'default' : 'books'}
              icon={tab === 'returned' ? RotateCcw : BookOpen}
              title={
                tab === 'borrowed'
                  ? 'No books borrowed'
                  : tab === 'due'
                    ? 'Nothing due soon'
                    : 'No returned books yet'
              }
              message={
                tab === 'returned'
                  ? 'Books you return will be listed here with their history.'
                  : 'Browse the catalog to borrow a book.'
              }
              action={
                tab !== 'returned' ? (
                  <Link className="btn btn-primary" to="/student/books">
                    Browse catalog
                  </Link>
                ) : null
              }
            />
          ) : (
            <>
              {/* Desktop: full table */}
              <div className="table-responsive hide-mobile">
                <table className="table">
                  <thead>
                    <tr>
                      <th>Book</th>
                      <th>Issue Date</th>
                      <th>Due Date</th>
                      <th>{tab === 'returned' ? 'Returned' : 'Status'}</th>
                      <th>Fine</th>
                      <th className="text-right">Action</th>
                    </tr>
                  </thead>
                  <tbody>
                    {rows.map((tx) => {
                      const book = getBook(state, tx.bookId)
                      const status = effectiveStatus(tx)
                      return (
                        <tr key={tx.id}>
                          <td>
                            <div className="table-cell-book">
                              <div className="cover-sm-row">
                                <BookCover book={book} size="sm" />
                              </div>
                              <div>
                                <div className="book-title">{book.title}</div>
                                <div className="book-meta">
                                  {book.author} · {branchName(state, book.branchId)}
                                </div>
                              </div>
                            </div>
                          </td>
                          <td>{formatDate(tx.issueDate)}</td>
                          <td className="strong">{formatDate(tx.dueDate)}</td>
                          <td>
                            {tab === 'returned' ? (
                              <span className="text-sm">{formatDate(tx.returnDate)}</span>
                            ) : (
                              <div className="stack stack-sm">
                                <Badge status={status === 'overdue' ? 'overdue' : status === 'issued' ? 'issued' : 'due'}>
                                  {status === 'overdue' ? 'Overdue' : 'Issued'}
                                </Badge>
                                <span className="text-xs text-muted">{dueLabel(tx)}</span>
                              </div>
                            )}
                          </td>
                          <td>{tx.fine > 0 ? <span className="text-danger font-semibold">₹{tx.fine}</span> : '—'}</td>
                          <td>
                            <div className="actions-cell">
                              <button
                                className="btn btn-secondary btn-sm"
                                onClick={() => navigate(`/student/books/${book.id}`)}
                              >
                                View
                              </button>
                              {tx.status === 'issued' && (
                                <button className="btn btn-primary btn-sm" onClick={() => setReturning(tx)}>
                                  <RotateCcw size={13} /> Return
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

              {/* Phones: one compact card per loan (swipe left for actions) */}
              <div className="loan-list only-mobile">
                {rows.map((tx) => {
                  const book = getBook(state, tx.bookId)
                  const status = effectiveStatus(tx)
                  const isReturned = tab === 'returned'
                  const canReturn = tx.status === 'issued'
                  return (
                    <SwipeableRow
                      key={tx.id}
                      revealWidth={canReturn ? 176 : 104}
                      actions={
                        <>
                          <button
                            className="btn btn-secondary btn-sm"
                            onClick={() => navigate(`/student/books/${book.id}`)}
                          >
                            View
                          </button>
                          {canReturn && (
                            <button className="btn btn-primary btn-sm" onClick={() => setReturning(tx)}>
                              <RotateCcw size={13} /> Return
                            </button>
                          )}
                        </>
                      }
                    >
                      <div className="loan-card">
                        <div className="cover-thumb">
                          <BookCover book={book} size="sm" />
                        </div>
                        <div className="loan-body">
                          <div className="loan-title">{book.title}</div>
                          <div className="loan-meta">
                            {isReturned
                              ? `Issued ${formatDate(tx.issueDate)} · Returned ${formatDate(tx.returnDate)}`
                              : `Issued ${formatDate(tx.issueDate)} · Due ${formatDate(tx.dueDate)}`}
                          </div>
                          <div className="loan-badges">
                            {!isReturned && (
                              <Badge status={status === 'overdue' ? 'overdue' : status === 'issued' ? 'issued' : 'due'}>
                                {status === 'overdue' ? 'Overdue' : 'Issued'}
                              </Badge>
                            )}
                            {!isReturned && status !== 'overdue' && (
                              <span className="loan-soon">{dueLabel(tx)}</span>
                            )}
                            {tx.fine > 0 && <span className="loan-fine">Fine ₹{tx.fine}</span>}
                          </div>
                          <div className="loan-actions">
                            <button
                              className="btn btn-secondary btn-sm"
                              onClick={() => navigate(`/student/books/${book.id}`)}
                            >
                              View
                            </button>
                            {canReturn && (
                              <button className="btn btn-primary btn-sm" onClick={() => setReturning(tx)}>
                                <RotateCcw size={13} /> Return
                              </button>
                            )}
                          </div>
                        </div>
                      </div>
                    </SwipeableRow>
                  )
                })}
              </div>
            </>
          )}
        </div>
      </div>

      <ConfirmDialog
        open={Boolean(returning)}
        title="Return this book?"
        danger={false}
        confirmLabel="Return Book"
        message={
          returning
            ? `"${getBook(state, returning.bookId)?.title}" will be marked as returned today (${formatDate(todayISO())}).`
            : ''
        }
        onClose={() => setReturning(null)}
        onConfirm={confirmReturn}
      />
    </>
  )
}
