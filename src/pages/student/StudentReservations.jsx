import { useState } from 'react'
import { Link, useNavigate } from 'react-router-dom'
import { Bookmark, XCircle } from 'lucide-react'
import { useLibrary } from '../../store/LibraryContext.jsx'
import { useAuth } from '../../store/AuthContext.jsx'
import { usePageTitle } from '../../store/PageTitleContext.jsx'
import { useToast } from '../../store/ToastContext.jsx'
import Badge from '../../components/ui/Badge.jsx'
import BookCover from '../../components/books/BookCover.jsx'
import EmptyState from '../../components/ui/EmptyState.jsx'
import ConfirmDialog from '../../components/ui/ConfirmDialog.jsx'
import { getBook, branchName } from '../../utils/selectors.js'
import { formatDate } from '../../utils/format.js'
import { simulatedLatency } from '../../services/api.js'

const STATUS_LABEL = {
  waiting: 'Waiting',
  ready: 'Ready for Pickup',
  fulfilled: 'Fulfilled',
  expired: 'Expired',
}

export default function StudentReservations() {
  usePageTitle('Reservations')
  const { state, cancelReservation, track } = useLibrary()
  const { user } = useAuth()
  const { toast } = useToast()
  const navigate = useNavigate()

  const [tab, setTab] = useState('active')
  const [cancelling, setCancelling] = useState(null)

  const mine = state.reservations.filter((r) => r.studentId === user.id)
  const active = mine.filter((r) => r.status === 'waiting' || r.status === 'ready')
  const history = mine.filter((r) => r.status === 'expired' || r.status === 'fulfilled')
  const rows = tab === 'active' ? active : history

  function confirmCancel() {
    cancelReservation(cancelling.id) // optimistic
    track(simulatedLatency(350))
    const book = getBook(state, cancelling.bookId)
    setCancelling(null)
    toast(`Reservation for "${book.title}" cancelled.`, 'info')
  }

  return (
    <>
      <div className="page-header">
        <div>
          <h1>Reservations</h1>
          <p className="page-sub">
            Queue for books that are currently issued to other students.
          </p>
        </div>
        <div className="page-actions">
          <Link className="btn btn-secondary" to="/student/books">Find books</Link>
        </div>
      </div>

      <div className="tabs">
        <button className={`tab ${tab === 'active' ? 'active' : ''}`} onClick={() => setTab('active')}>
          Active<span className="count">{active.length}</span>
        </button>
        <button className={`tab ${tab === 'history' ? 'active' : ''}`} onClick={() => setTab('history')}>
          History<span className="count">{history.length}</span>
        </button>
      </div>

      <div className="card">
        <div className="card-body flush">
          {rows.length === 0 ? (
            <EmptyState
              icon={Bookmark}
              title={tab === 'active' ? 'No active reservations' : 'No reservation history'}
              message={
                tab === 'active'
                  ? 'When every copy of a book is issued you can reserve it and join the queue.'
                  : 'Expired and fulfilled reservations will appear here.'
              }
              action={
                tab === 'active' ? (
                  <Link className="btn btn-primary" to="/student/books">Browse catalog</Link>
                ) : null
              }
            />
          ) : (
            <div className="table-responsive">
              <table className="table">
                <thead>
                  <tr>
                    <th>Book</th>
                    <th>Reserved On</th>
                    <th>Queue Position</th>
                    <th>Status</th>
                    <th>Expected / Pickup</th>
                    <th className="text-right">Action</th>
                  </tr>
                </thead>
                <tbody>
                  {rows.map((r) => {
                    const book = getBook(state, r.bookId)
                    return (
                      <tr key={r.id}>
                        <td>
                          <div className="table-cell-book">
                            <div className="cover-sm-row">
                              <BookCover book={book} size="sm" />
                            </div>
                            <div>
                              <div className="book-title">{book.title}</div>
                              <div className="book-meta">{branchName(state, book.branchId)}</div>
                            </div>
                          </div>
                        </td>
                        <td>{formatDate(r.reservationDate)}</td>
                        <td className="strong">#{r.queuePosition}</td>
                        <td>
                          <Badge
                            status={
                              r.status === 'ready'
                                ? 'ready'
                                : r.status === 'waiting'
                                  ? 'waiting'
                                  : r.status === 'fulfilled'
                                    ? 'fulfilled'
                                    : 'expired'
                            }
                          >
                            {STATUS_LABEL[r.status]}
                          </Badge>
                        </td>
                        <td>
                          {r.status === 'ready'
                            ? `Pickup by ${formatDate(r.pickupDeadline)}`
                            : r.status === 'waiting'
                              ? `Expected ${formatDate(r.expectedAvailability)}`
                              : '—'}
                        </td>
                        <td>
                          <div className="actions-cell">
                            <button
                              className="btn btn-secondary btn-sm"
                              onClick={() => navigate(`/student/books/${book.id}`)}
                            >
                              View
                            </button>
                            {(r.status === 'waiting' || r.status === 'ready') && (
                              <button
                                className="btn btn-danger btn-sm"
                                onClick={() => setCancelling(r)}
                              >
                                <XCircle size={13} /> Cancel
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

      <ConfirmDialog
        open={Boolean(cancelling)}
        title="Cancel reservation?"
        confirmLabel="Cancel Reservation"
        message={
          cancelling
            ? `Your place in the queue for "${getBook(state, cancelling.bookId)?.title}" will be released.`
            : ''
        }
        onClose={() => setCancelling(null)}
        onConfirm={confirmCancel}
      />
    </>
  )
}
