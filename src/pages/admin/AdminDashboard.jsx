import { useRef, useState } from 'react'
import { Link } from 'react-router-dom'
import {
  BookOpen,
  Layers,
  ArrowLeftRight,
  AlertTriangle,
  IndianRupee,
  Plus,
  Upload,
  Megaphone,
  GripVertical,
  Bell,
  TrendingUp,
} from 'lucide-react'
import { useLibrary } from '../../store/LibraryContext.jsx'
import { useAuth } from '../../store/AuthContext.jsx'
import { usePageTitle } from '../../store/PageTitleContext.jsx'
import StatCard from '../../components/ui/StatCard.jsx'
import Badge from '../../components/ui/Badge.jsx'
import BookCover from '../../components/books/BookCover.jsx'
import EmptyState from '../../components/ui/EmptyState.jsx'
import {
  adminStats,
  getStudent,
  getBook,
  overdueLoans,
  effectiveStatus,
  liveFine,
  seriesTrend,
} from '../../utils/selectors.js'
import { formatDate, formatLongDate, formatNumber, timeGreeting, daysRemaining, relativeDay, getInitials } from '../../utils/format.js'

const STAT_ORDER_KEY = 'lms.admin.statOrder'
const STAT_IDS = ['total', 'available', 'issued', 'overdue', 'fines']

function loadStatOrder() {
  try {
    const saved = JSON.parse(localStorage.getItem(STAT_ORDER_KEY))
    if (Array.isArray(saved) && saved.length === STAT_IDS.length && STAT_IDS.every((id) => saved.includes(id))) {
      return saved
    }
  } catch {
    /* corrupted storage - fall back to the default order */
  }
  return STAT_IDS
}

export default function AdminDashboard() {
  usePageTitle('Library Overview', 'Catalog, loans and dues at a glance')
  const { state } = useLibrary()
  const { user } = useAuth()

  const [statOrder, setStatOrder] = useState(loadStatOrder)
  const [dragId, setDragId] = useState(null)
  const dragIdRef = useRef(null)

  const stats = adminStats(state)
  const overdue = overdueLoans(state)
    .map((tx) => ({ tx, days: -daysRemaining(tx.dueDate) }))
    .sort((a, b) => b.days - a.days)

  const announcements = [...state.notifications]
    .filter((n) => n.type === 'announcement')
    .sort((a, b) => (a.time < b.time ? 1 : -1))
    .slice(0, 5)

  const recent = [...state.transactions]
    .sort((a, b) => {
      const da = a.returnDate || a.issueDate
      const db = b.returnDate || b.issueDate
      return db < da ? -1 : 1
    })
    .slice(0, 8)

  const popular = state.analytics.popularBooks
  const atRisk = overdue.reduce((sum, { tx }) => sum + liveFine(tx), 0)

  // ---- stat card trends / sparklines (real series from the mock data) ----
  const issuedSpark = state.analytics.monthlyBorrowing.map((m) => m.issued)
  const pendingSpark = state.analytics.fineTrend.map((f) => f.pending)
  const issuedTrend = seriesTrend(issuedSpark, true)
  const pendingTrend = seriesTrend(pendingSpark, false)

  const statDefs = {
    total: (
      <StatCard icon={Layers} tone="blue" label="Total Books" value={formatNumber(stats.totalCopies)} note={`${stats.totalTitles} titles in catalog`} />
    ),
    available: (
      <StatCard icon={BookOpen} tone="green" label="Available Books" value={formatNumber(stats.availableCopies)} note="Ready to be issued" />
    ),
    issued: (
      <StatCard icon={ArrowLeftRight} tone="cyan" label="Issued Books" value={formatNumber(stats.issuedCopies)} note={`${stats.activeReservations} active reservations`} trend={issuedTrend} />
    ),
    overdue: (
      <StatCard icon={AlertTriangle} tone="red" label="Overdue Books" value={formatNumber(stats.overdue)} note="Need follow-up" />
    ),
    fines: (
      <StatCard icon={IndianRupee} tone="amber" label="Outstanding Fines" value={`₹${formatNumber(stats.pendingFines)}`} note={`₹${formatNumber(stats.collectedFines)} collected`} trend={pendingTrend} />
    ),
  }

  /** Drag-to-reorder: move `source` next to `target` and persist. */
  function moveStat(source, target) {
    setStatOrder((current) => {
      const next = current.filter((id) => id !== source)
      const at = next.indexOf(target)
      next.splice(at < 0 ? next.length : at, 0, source)
      try {
        localStorage.setItem(STAT_ORDER_KEY, JSON.stringify(next))
      } catch {
        /* storage unavailable - order applies for this session */
      }
      return next
    })
  }

  return (
    <>
      <div className="page-header">
        <div>
          <h1>
            {timeGreeting()}, <span className="greeting-name">{user?.name?.split(' ')[0] || 'Librarian'}</span>
          </h1>
          <p className="page-sub">
            Here&apos;s your library snapshot for {formatLongDate()}
          </p>
        </div>
        <div className="page-actions">
          <Link className="btn btn-secondary" to="/admin/import">
            <Upload size={16} /> Bulk Import
          </Link>
          <Link className="btn btn-primary" to="/admin/books/new">
            <Plus size={16} /> Add Book
          </Link>
        </div>
      </div>

      {/* Stat cards: drag any card by its grip to reorder (saved locally). */}
      <div className="grid mb-4 stat-grid">
        {statOrder.map((id) => (
          <div
            key={id}
            className={`stat-drag ${dragId === id ? 'dragging' : ''}`}
            draggable
            onDragStart={(e) => {
              dragIdRef.current = id
              setDragId(id)
              e.dataTransfer.effectAllowed = 'move'
            }}
            onDragEnd={() => {
              dragIdRef.current = null
              setDragId(null)
            }}
            onDragOver={(e) => {
              e.preventDefault()
              if (dragIdRef.current && dragIdRef.current !== id) moveStat(dragIdRef.current, id)
            }}
            onDrop={(e) => e.preventDefault()}
          >
            <span className="drag-grip" aria-hidden="true" data-tip="Drag to reorder">
              <GripVertical size={14} />
            </span>
            {statDefs[id]}
          </div>
        ))}
      </div>

      <section className="card mb-4">
          <div className="card-header">
            <h3><span className="card-ico info"><Bell size={15} /></span>Recent Announcements</h3>
            <div className="card-actions">
              <Badge tone="info">{announcements.length}</Badge>
              <Link className="btn btn-ghost btn-sm" to="/admin/notifications">View all</Link>
            </div>
          </div>
          <div className="card-body flush ann-grid">
            {announcements.length === 0 ? (
              <EmptyState
                kind="bell"
                icon={Megaphone}
                title="No announcements yet"
                message="Publish your first announcement to see it here."
              />
            ) : (
              announcements.slice(0, 3).map((a) => (
                <Link key={a.id} to="/admin/notifications" className="notif-item notif-link">
                  <div className="notif-icon" style={{ background: 'var(--info-soft)', color: 'var(--info)' }}>
                    <Megaphone size={17} />
                  </div>
                  <div className="notif-body">
                    <div className="notif-head">
                      <span className="notif-title">{a.title}</span>
                      <span className="notif-time">{relativeDay(a.time)}</span>
                    </div>
                    <div className="notif-message">{a.message}</div>
                  </div>
                </Link>
              ))
            )}
          </div>
        </section>

        <section className="card mb-4">
        <div className="card-header">
          <h3><span className="card-ico cyan"><ArrowLeftRight size={15} /></span>Recent Transactions</h3>
          <div className="card-actions">
            <Badge tone="info">{recent.length} latest</Badge>
            <Link className="btn btn-ghost btn-sm" to="/admin/transactions">View all</Link>
          </div>
        </div>
        <div className="card-body flush">
          <div className="table-responsive">
            <table className="table">
              <thead>
                <tr>
                  <th>Student</th>
                  <th>Book</th>
                  <th>Action</th>
                  <th>Date</th>
                  <th>Status</th>
                </tr>
              </thead>
              <tbody>
                {recent.map((tx) => {
                  const student = getStudent(state, tx.studentId)
                  const book = getBook(state, tx.bookId)
                  const returned = tx.status === 'returned'
                  return (
                    <tr key={tx.id}>
                      <td>
                        <div className="cell-user">
                          <span className="avatar avatar-sm">{getInitials(student?.name || '')}</span>
                          <span className="strong">{student?.name || '—'}</span>
                        </div>
                      </td>
                      <td>
                        <div className="table-cell-book">
                          <div className="cover-sm-row">
                            <BookCover book={book} size="sm" />
                          </div>
                          <div>
                            <div className="book-title">{book?.title}</div>
                            <div className="book-meta">{student?.studentId}</div>
                          </div>
                        </div>
                      </td>
                      <td>
                        <Badge tone={returned ? 'success' : 'info'}>{returned ? 'Returned' : 'Issued'}</Badge>
                      </td>
                      <td>{formatDate(tx.returnDate || tx.issueDate)}</td>
                      <td>
                        <Badge status={returned ? 'returned' : effectiveStatus(tx) === 'overdue' ? 'overdue' : 'issued'}>
                          {returned ? 'Completed' : effectiveStatus(tx) === 'overdue' ? 'Overdue' : 'Active'}
                        </Badge>
                      </td>
                    </tr>
                  )
                })}
              </tbody>
            </table>
          </div>
        </div>
      </section>

      <div className="grid grid-split mb-4" style={{ gridTemplateColumns: 'minmax(0, 1.3fr) minmax(0, 1fr)' }}>
        <section className="card">
          <div className="card-header">
            <h3><span className="card-ico danger"><AlertTriangle size={15} /></span>Overdue Books</h3>
            <div className="card-actions">
              <span className="text-xs text-muted">₹{formatNumber(atRisk)} at risk</span>
              <Badge tone="danger">{overdue.length} overdue</Badge>
            </div>
          </div>
          <div className="card-body flush">
            {overdue.length === 0 ? (
              <EmptyState icon={AlertTriangle} title="No overdue books" message="Every loan is within its due date." />
            ) : (
              <div className="table-responsive">
                <table className="table">
                  <thead>
                    <tr>
                      <th>Student</th>
                      <th>Book</th>
                      <th>Due Date</th>
                      <th>Days Overdue</th>
                      <th>Fine</th>
                    </tr>
                  </thead>
                  <tbody>
                    {overdue.map(({ tx, days }) => {
                      const student = getStudent(state, tx.studentId)
                      const book = getBook(state, tx.bookId)
                      return (
                        <tr key={tx.id}>
                          <td className="strong">{student?.name}</td>
                          <td>{book?.title}</td>
                          <td>{formatDate(tx.dueDate)}</td>
                          <td>
                            <Badge status="overdue">{days} day{days === 1 ? '' : 's'}</Badge>
                          </td>
                          <td className="font-bold text-danger">₹{liveFine(tx)}</td>
                        </tr>
                      )
                    })}
                  </tbody>
                </table>
              </div>
            )}
          </div>
        </section>

        <section className="card">
          <div className="card-header">
            <h3><span className="card-ico success"><TrendingUp size={15} /></span>Popular Books</h3>
            <div className="card-actions"><span className="text-xs text-muted">Last semester</span></div>
          </div>
          <div className="card-body stack stack-md">
            {popular.slice(0, 5).map((item, i) => {
              const max = popular[0].borrows
              const pct = Math.round((item.borrows / max) * 100)
              return (
                <div key={item.title}>
                  <div className="row-between text-sm mb-2">
                    <span className="pop-title">
                      <span className={`rank-num ${i === 0 ? 'top' : ''}`}>{i + 1}</span>
                      <span className="font-semibold">{item.title}</span>
                    </span>
                    <span className="text-muted">{item.borrows} borrows · {pct}%</span>
                  </div>
                  <div className="progress-track">
                    <div className="progress-fill grad" style={{ width: `${pct}%` }} />
                  </div>
                </div>
              )
            })}
          </div>
        </section>
      </div>
    </>
  )
}
