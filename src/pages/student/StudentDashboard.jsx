import { Link, useNavigate } from 'react-router-dom'
import {
  BookOpen,
  CalendarClock,
  Bookmark,
  IndianRupee,
  ArrowRight,
} from 'lucide-react'
import { useLibrary } from '../../store/LibraryContext.jsx'
import { useAuth } from '../../store/AuthContext.jsx'
import { usePageTitle } from '../../store/PageTitleContext.jsx'
import StatCard from '../../components/ui/StatCard.jsx'
import Badge from '../../components/ui/Badge.jsx'
import BookCover from '../../components/books/BookCover.jsx'
import BookCard from '../../components/books/BookCard.jsx'
import EmptyState from '../../components/ui/EmptyState.jsx'
import {
  studentStats,
  activeLoansOf,
  getBook,
  effectiveStatus,
  daysRemaining,
  branchName,
} from '../../utils/selectors.js'
import { formatDate, formatCurrency, timeGreeting } from '../../utils/format.js'

/** 'Due in 3 days' / 'Due today' chip for loans inside the 7-day window. */
function dueSoonNote(tx) {
  const days = daysRemaining(tx.dueDate)
  if (days > 7 || effectiveStatus(tx) === 'overdue') return null
  return days <= 0 ? 'Due today' : `Due in ${days} day${days === 1 ? '' : 's'}`
}

export default function StudentDashboard() {
  usePageTitle('Dashboard')
  const { state } = useLibrary()
  const { user } = useAuth()
  const navigate = useNavigate()

  const stats = studentStats(state, user.id)
  const active = activeLoansOf(state, user.id)
  const recentlyAdded = [...state.books].slice(0, 8)

  return (
    <>
      <div className="page-header">
        <div>
          <h1>
            {timeGreeting()}, <span className="greeting-name">{user.name.split(' ')[0]}</span>
          </h1>
          <p className="page-sub">
            {stats.dueSoon > 0
              ? `You have ${stats.dueSoon} book${stats.dueSoon === 1 ? '' : 's'} due this week.`
              : 'Nothing is due this week — enjoy your reading.'}
          </p>
        </div>
      </div>

      <div className="grid grid-4 mb-4">
        <StatCard
          icon={BookOpen}
          tone="blue"
          label="Currently Borrowed"
          value={`${stats.borrowed} Book${stats.borrowed === 1 ? '' : 's'}`}
          note={stats.overdue ? `${stats.overdue} overdue` : 'All within loan period'}
        />
        <StatCard
          icon={CalendarClock}
          tone="amber"
          label="Due Soon"
          value={`${stats.dueSoon} Book${stats.dueSoon === 1 ? '' : 's'}`}
          note="Within the next 7 days"
        />
        <StatCard
          icon={Bookmark}
          tone="cyan"
          label="Reservations"
          value={`${stats.reservations} Active`}
          note="Waiting or ready for pickup"
        />
        <StatCard
          icon={IndianRupee}
          tone="red"
          label="Outstanding Fine"
          value={formatCurrency(stats.fine)}
          note={stats.fine ? 'Clear at the library counter' : 'No dues pending'}
        />
      </div>

      {/* ------------- Currently borrowed ------------- */}
      <section className="card dashboard-sec mb-4">
        <div className="card-header">
          <div>
            <h3>Currently Borrowed</h3>
            <div className="card-sub">
              {stats.dueSoon > 0
                ? `Books currently with you · ${stats.dueSoon} due this week`
                : 'Books currently with you'}
            </div>
          </div>
          <div className="card-actions">
            <Badge tone="gray">{active.length} active</Badge>
          </div>
        </div>
        <div className="card-body flush">
          {active.length === 0 ? (
            <EmptyState
              kind="books"
              icon={BookOpen}
              title="No books borrowed yet"
              message="Browse the catalog and borrow your first book."
              action={
                <Link className="btn btn-primary" to="/student/books">
                  Browse catalog
                </Link>
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
                      <th>Branch</th>
                      <th>Issued</th>
                      <th>Due Date</th>
                      <th>Status</th>
                      <th></th>
                    </tr>
                  </thead>
                  <tbody>
                    {active.map((tx) => {
                      const book = getBook(state, tx.bookId)
                      const overdue = effectiveStatus(tx) === 'overdue'
                      return (
                        <tr key={tx.id}>
                          <td>
                            <div className="table-cell-book">
                              <div className="cover-sm-row">
                                <BookCover book={book} size="sm" />
                              </div>
                              <div>
                                <div className="book-title">{book.title}</div>
                                <div className="book-meta">{book.author}</div>
                              </div>
                            </div>
                          </td>
                          <td>{branchName(state, book.branchId)}</td>
                          <td>{formatDate(tx.issueDate)}</td>
                          <td className="strong">
                            {formatDate(tx.dueDate)}
                            {dueSoonNote(tx) && <div className="due-soon-note">{dueSoonNote(tx)}</div>}
                          </td>
                          <td>
                            <Badge status={overdue ? 'overdue' : 'issued'}>
                              {overdue ? 'Overdue' : 'Issued'}
                            </Badge>
                          </td>
                          <td className="text-right">
                            <button
                              className="btn btn-secondary btn-sm"
                              onClick={() => navigate(`/student/books/${book.id}`)}
                            >
                              View
                            </button>
                          </td>
                        </tr>
                      )
                    })}
                  </tbody>
                </table>
              </div>

              {/* Phones: one compact card per loan */}
              <div className="loan-list only-mobile">
                {active.map((tx) => {
                  const book = getBook(state, tx.bookId)
                  const overdue = effectiveStatus(tx) === 'overdue'
                  return (
                    <div className="loan-card" key={tx.id}>
                      <div className="cover-thumb">
                        <BookCover book={book} size="sm" />
                      </div>
                      <div className="loan-body">
                        <div className="loan-title">{book.title}</div>
                        <div className="loan-meta">
                          Due {formatDate(tx.dueDate)} · {branchName(state, book.branchId)}
                        </div>
                        <div className="loan-badges">
                          <Badge status={overdue ? 'overdue' : 'issued'}>
                            {overdue ? 'Overdue' : 'Issued'}
                          </Badge>
                          {dueSoonNote(tx) && <span className="loan-soon">{dueSoonNote(tx)}</span>}
                          {overdue && <span className="loan-fine">Fine applies</span>}
                        </div>
                      </div>
                      <button
                        className="icon-btn"
                        onClick={() => navigate(`/student/books/${book.id}`)}
                        aria-label={`View ${book.title}`}
                      >
                        <ArrowRight size={17} />
                      </button>
                    </div>
                  )
                })}
              </div>
            </>
          )}
        </div>
      </section>

      {/* ------------- Recently added ------------- */}
      <section>
        <div className="row-between mb-3">
          <div>
            <div className="section-title">Recently Added</div>
            <div className="section-sub">Fresh arrivals in the library catalog</div>
          </div>
          <Link className="btn btn-secondary btn-sm" to="/student/books">
            Browse all <ArrowRight size={14} />
          </Link>
        </div>
        <div className="book-grid">
          {recentlyAdded.map((book) => (
            <BookCard key={book.id} book={book} />
          ))}
        </div>
      </section>
    </>
  )
}

