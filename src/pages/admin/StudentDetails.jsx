import { useState } from 'react'
import { Link, useParams, useNavigate } from 'react-router-dom'
import {
  ArrowLeft,
  Mail,
  Phone,
  GraduationCap,
  IdCard,
  BookOpen,
  History,
  Bookmark,
  IndianRupee,
} from 'lucide-react'
import { useLibrary } from '../../store/LibraryContext.jsx'
import { usePageTitle } from '../../store/PageTitleContext.jsx'
import { useToast } from '../../store/ToastContext.jsx'
import Badge from '../../components/ui/Badge.jsx'
import BookCover from '../../components/books/BookCover.jsx'
import EmptyState from '../../components/ui/EmptyState.jsx'
import Breadcrumbs from '../../components/ui/Breadcrumbs.jsx'
import StatCard from '../../components/ui/StatCard.jsx'
import { getInitials, formatDate } from '../../utils/format.js'
import {
  activeLoansOf,
  getBook,
  effectiveStatus,
  dueLabel,
  reservationsOf,
  outstandingFines,
  totalOf,
  branchName,
} from '../../utils/selectors.js'

export default function StudentDetails() {
  const { id } = useParams()
  const navigate = useNavigate()
  const { state, updateStudent } = useLibrary()
  const { toast } = useToast()
  usePageTitle('Student Details')

  const [tab, setTab] = useState('current')
  const student = state.students.find((s) => s.id === id)

  if (!student) {
    return (
      <div className="card">
        <EmptyState
          icon={BookOpen}
          title="Student not found"
          message="This student record may have been removed."
          action={<Link className="btn btn-primary" to="/admin/students">Back to students</Link>}
        />
      </div>
    )
  }

  const active = activeLoansOf(state, student.id)
  const history = state.transactions
    .filter((t) => t.studentId === student.id && t.status === 'returned')
    .sort((a, b) => (b.returnDate < a.returnDate ? -1 : 1))
  const reservations = reservationsOf(state, student.id)
  const fines = outstandingFines(state, student.id)
  const fineTotal = totalOf(fines)

  const TABS = [
    { id: 'current', label: 'Current Books', count: active.length },
    { id: 'history', label: 'Borrowing History', count: history.length },
    { id: 'reservations', label: 'Reservations', count: reservations.length },
    { id: 'fines', label: 'Outstanding Fines', count: fines.length },
  ]

  return (
    <>
      <Breadcrumbs
        items={[
          { label: 'Students', to: '/admin/students' },
          { label: student.name },
        ]}
      />
      <div className="page-header">
        <div>
          <Link className="btn btn-ghost btn-sm mb-2 hide-mobile" to="/admin/students">
            <ArrowLeft size={15} /> Back to students
          </Link>
          <div className="row" style={{ gap: 14 }}>
            <div className="avatar avatar-lg">{getInitials(student.name)}</div>
            <div>
              <h1>{student.name}</h1>
              <p className="page-sub">
                {student.studentId} · {student.branch} · {student.program} Semester {student.semester}
              </p>
            </div>
          </div>
        </div>
        <div className="page-actions">
          <Badge status={student.status}>{student.status === 'active' ? 'Active' : 'Suspended'}</Badge>
        </div>
      </div>

      <div className="grid grid-4 mb-4">
        <StatCard icon={BookOpen} tone="blue" label="Books Issued" value={active.length} note="Currently borrowed" />
        <StatCard icon={History} tone="green" label="Books Returned" value={history.length} note="Lifetime total" />
        <StatCard icon={Bookmark} tone="cyan" label="Reservations" value={reservations.filter((r) => r.status === 'waiting' || r.status === 'ready').length} note="Active queue" />
        <StatCard icon={IndianRupee} tone="red" label="Outstanding Fine" value={`₹${fineTotal}`} note={fineTotal ? 'Pending payment' : 'No dues'} />
      </div>

      <div className="tabs">
        {TABS.map((t) => (
          <button key={t.id} className={`tab ${tab === t.id ? 'active' : ''}`} onClick={() => setTab(t.id)}>
            {t.label}<span className="count">{t.count}</span>
          </button>
        ))}
      </div>

      <div className="grid grid-split mb-4" style={{ gridTemplateColumns: 'minmax(0, 1fr) minmax(0, 2fr)' }}>
        <section className="card card-pad">
          <div className="section-title mb-3">Personal Information</div>
          <div className="info-list">
            <div className="info-row"><span className="info-label row"><IdCard size={14} /> Student ID</span><span className="info-value">{student.studentId}</span></div>
            <div className="info-row"><span className="info-label row"><Mail size={14} /> Email</span><span className="info-value">{student.email}</span></div>
            <div className="info-row"><span className="info-label row"><Phone size={14} /> Contact</span><span className="info-value">{student.contact}</span></div>
            <div className="info-row"><span className="info-label row"><GraduationCap size={14} /> Branch</span><span className="info-value">{student.branch}</span></div>
            <div className="info-row"><span className="info-label">Joined</span><span className="info-value">{formatDate(student.joinedDate)}</span></div>
          </div>
        </section>

        <section className="card">
          <div className="card-header"><h3>Account Activity</h3></div>
          <div className="card-body flush">
            {tab === 'current' && (
              active.length === 0 ? (
                <EmptyState icon={BookOpen} title="No books issued" message="This student has nothing borrowed right now." />
              ) : (
                <div className="table-responsive">
                  <table className="table">
                    <thead><tr><th>Book</th><th>Issued</th><th>Due</th><th>Status</th></tr></thead>
                    <tbody>
                      {active.map((tx) => {
                        const book = getBook(state, tx.bookId)
                        const status = effectiveStatus(tx)
                        return (
                          <tr key={tx.id}>
                            <td className="strong">{book?.title}</td>
                            <td>{formatDate(tx.issueDate)}</td>
                            <td>{formatDate(tx.dueDate)}</td>
                            <td>
                              <Badge status={status === 'overdue' ? 'overdue' : 'issued'}>
                                {status === 'overdue' ? `Overdue · ${dueLabel(tx)}` : 'Issued'}
                              </Badge>
                            </td>
                          </tr>
                        )
                      })}
                    </tbody>
                  </table>
                </div>
              )
            )}

            {tab === 'history' && (
              history.length === 0 ? (
                <EmptyState icon={History} title="No history" message="No books have been returned yet." />
              ) : (
                <div className="table-responsive">
                  <table className="table">
                    <thead><tr><th>Book</th><th>Issued</th><th>Returned</th><th>Fine</th></tr></thead>
                    <tbody>
                      {history.map((tx) => (
                        <tr key={tx.id}>
                          <td className="strong">{getBook(state, tx.bookId)?.title}</td>
                          <td>{formatDate(tx.issueDate)}</td>
                          <td>{formatDate(tx.returnDate)}</td>
                          <td>{tx.fine > 0 ? <span className="text-danger font-semibold">₹{tx.fine}</span> : '—'}</td>
                        </tr>
                      ))}
                    </tbody>
                  </table>
                </div>
              )
            )}

            {tab === 'reservations' && (
              reservations.length === 0 ? (
                <EmptyState icon={Bookmark} title="No reservations" message="This student has not reserved any books." />
              ) : (
                <div className="table-responsive">
                  <table className="table">
                    <thead><tr><th>Book</th><th>Reserved</th><th>Queue</th><th>Status</th></tr></thead>
                    <tbody>
                      {reservations.map((r) => (
                        <tr key={r.id}>
                          <td className="strong">{getBook(state, r.bookId)?.title}</td>
                          <td>{formatDate(r.reservationDate)}</td>
                          <td>#{r.queuePosition}</td>
                          <td><Badge status={r.status}>{r.status}</Badge></td>
                        </tr>
                      ))}
                    </tbody>
                  </table>
                </div>
              )
            )}

            {tab === 'fines' && (
              fines.length === 0 ? (
                <EmptyState icon={IndianRupee} title="No outstanding fines" message="All dues are cleared." />
              ) : (
                <div className="table-responsive">
                  <table className="table">
                    <thead><tr><th>Book</th><th>Due</th><th>Overdue Days</th><th>Fine</th></tr></thead>
                    <tbody>
                      {fines.map((f) => (
                        <tr key={f.id}>
                          <td className="strong">{getBook(state, f.bookId)?.title}</td>
                          <td>{formatDate(f.dueDate)}</td>
                          <td>{f.overdueDays}</td>
                          <td className="text-danger font-bold">₹{f.amount}</td>
                        </tr>
                      ))}
                    </tbody>
                  </table>
                </div>
              )
            )}
          </div>
        </section>
      </div>

      <div className="row wrap" style={{ gap: 10 }}>
        <button className="btn btn-secondary" onClick={() => navigate('/admin/transactions')}>
          Manage transactions
        </button>
        <button
          className={`btn ${student.status === 'active' ? 'btn-danger' : 'btn-primary'}`}
          onClick={() => {
            updateStudent({ id: student.id, status: student.status === 'active' ? 'suspended' : 'active' })
            toast(`${student.name} ${student.status === 'active' ? 'suspended' : 'activated'}.`, 'info')
          }}
        >
          {student.status === 'active' ? 'Suspend account' : 'Activate account'}
        </button>
      </div>
    </>
  )
}
