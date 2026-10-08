import { useMemo, useState } from 'react'
import { Bookmark, CheckCircle2, XCircle, Bell } from 'lucide-react'
import { useLibrary } from '../../store/LibraryContext.jsx'
import { usePageTitle } from '../../store/PageTitleContext.jsx'
import { useToast } from '../../store/ToastContext.jsx'
import SearchBar from '../../components/ui/SearchBar.jsx'
import { Select } from '../../components/ui/Field.jsx'
import Badge from '../../components/ui/Badge.jsx'
import BookCover from '../../components/books/BookCover.jsx'
import EmptyState from '../../components/ui/EmptyState.jsx'
import { getStudent, getBook } from '../../utils/selectors.js'
import { formatDate, todayISO, addDays } from '../../utils/format.js'
import { simulatedLatency } from '../../services/api.js'

const TABS = [
  { id: 'all', label: 'All' },
  { id: 'waiting', label: 'Active' },
  { id: 'ready', label: 'Ready' },
  { id: 'fulfilled', label: 'Fulfilled' },
  { id: 'expired', label: 'Expired' },
]

const LABEL = {
  waiting: 'Waiting',
  ready: 'Ready for Pickup',
  fulfilled: 'Fulfilled',
  expired: 'Expired',
}

export default function AdminReservations() {
  usePageTitle('Reservations')
  const { state, setReservationStatus, track } = useLibrary()
  const { toast } = useToast()

  const [tab, setTab] = useState('all')
  const [query, setQuery] = useState('')

  const counts = useMemo(
    () =>
      TABS.reduce((acc, t) => {
        acc[t.id] = t.id === 'all' ? state.reservations.length : state.reservations.filter((r) => r.status === t.id).length
        return acc
      }, {}),
    [state.reservations],
  )

  const rows = useMemo(() => {
    const q = query.trim().toLowerCase()
    return state.reservations
      .filter((r) => {
        const student = getStudent(state, r.studentId)
        const book = getBook(state, r.bookId)
        if (tab !== 'all' && r.status !== tab) return false
        if (q && !`${student?.name} ${book?.title}`.toLowerCase().includes(q)) return false
        return true
      })
      .sort((a, b) => (a.reservationDate < b.reservationDate ? 1 : -1))
  }, [state, tab, query])

  function markReady(reservation) {
    setReservationStatus(reservation.id, 'ready') // optimistic
    track(simulatedLatency(350))
    toast('Student notified that the book is ready for pickup.', 'success')
  }

  function markFulfilled(reservation) {
    setReservationStatus(reservation.id, 'fulfilled')
    track(simulatedLatency(350))
    toast('Reservation marked as fulfilled.', 'success')
  }

  function markExpired(reservation) {
    setReservationStatus(reservation.id, 'expired')
    track(simulatedLatency(350))
    toast('Reservation expired.', 'info')
  }

  return (
    <>
      <div className="page-header">
        <div>
          <h1>Reservation Management</h1>
          <p className="page-sub">
            {counts.waiting} waiting · {counts.ready} ready for pickup · {counts.expired} expired
          </p>
        </div>
      </div>

      <div className="tabs">
        {TABS.map((t) => (
          <button key={t.id} className={`tab ${tab === t.id ? 'active' : ''}`} onClick={() => setTab(t.id)}>
            {t.label}<span className="count">{counts[t.id]}</span>
          </button>
        ))}
      </div>

      <div className="card card-pad mb-4">
        <SearchBar value={query} onChange={setQuery} placeholder="Search by student or book title..." />
      </div>

      <div className="card">
        <div className="card-body flush">
          {rows.length === 0 ? (
            <EmptyState
              icon={Bookmark}
              title="No reservations"
              message="No reservations match the selected filter."
            />
          ) : (
            <div className="table-responsive">
              <table className="table">
                <thead>
                  <tr>
                    <th>Student</th>
                    <th>Book</th>
                    <th>Reserved On</th>
                    <th>Queue</th>
                    <th>Status</th>
                    <th>Pickup Deadline</th>
                    <th className="text-right">Action</th>
                  </tr>
                </thead>
                <tbody>
                  {rows.map((r) => {
                    const student = getStudent(state, r.studentId)
                    const book = getBook(state, r.bookId)
                    return (
                      <tr key={r.id}>
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
                        <td>{formatDate(r.reservationDate)}</td>
                        <td className="strong">#{r.queuePosition}</td>
                        <td><Badge status={r.status}>{LABEL[r.status]}</Badge></td>
                        <td>
                          {r.status === 'ready'
                            ? formatDate(r.pickupDeadline)
                            : r.status === 'waiting'
                              ? `Expected ${formatDate(r.expectedAvailability)}`
                              : '—'}
                        </td>
                        <td>
                          <div className="actions-cell">
                            {r.status === 'waiting' && (
                              <button className="btn btn-primary btn-sm" onClick={() => markReady(r)}>
                                <Bell size={13} /> Mark Ready
                              </button>
                            )}
                            {r.status === 'ready' && (
                              <>
                                <button className="btn btn-success btn-sm" onClick={() => markFulfilled(r)}>
                                  <CheckCircle2 size={13} /> Fulfilled
                                </button>
                                <button className="btn btn-secondary btn-sm" onClick={() => markExpired(r)}>
                                  <XCircle size={13} /> Expire
                                </button>
                              </>
                            )}
                            {(r.status === 'fulfilled' || r.status === 'expired') && (
                              <span className="text-xs text-muted">Closed on {formatDate(r.pickupDeadline || addDays(r.reservationDate, 3))}</span>
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
      <p className="text-xs text-muted mt-3">
        When a reservation is marked ready, students get a pickup deadline of 3 days from today.
      </p>
    </>
  )
}
