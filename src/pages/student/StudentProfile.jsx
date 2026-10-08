import {
  Mail,
  Phone,
  GraduationCap,
  IdCard,
  BookOpen,
  RotateCcw,
  IndianRupee,
  Bookmark,
} from 'lucide-react'
import { useLibrary } from '../../store/LibraryContext.jsx'
import { useAuth } from '../../store/AuthContext.jsx'
import { usePageTitle } from '../../store/PageTitleContext.jsx'
import StatCard from '../../components/ui/StatCard.jsx'
import { getInitials, formatDate } from '../../utils/format.js'
import { studentStats, activeLoansOf, reservationsOf, outstandingFines } from '../../utils/selectors.js'

export default function StudentProfile() {
  usePageTitle('Profile')
  const { state } = useLibrary()
  const { user } = useAuth()

  const student = state.students.find((s) => s.id === user?.id) || user
  const stats = studentStats(state, user?.id)
  const active = activeLoansOf(state, user?.id)
  const reservations = reservationsOf(state, user?.id).filter(
    (r) => r.status === 'waiting' || r.status === 'ready',
  )
  const fines = outstandingFines(state, user?.id)

  return (
    <>
      <div className="page-header">
        <div>
          <h1>Profile</h1>
          <p className="page-sub">Your library account details and borrowing summary.</p>
        </div>
      </div>

      <div className="grid grid-split" style={{ gridTemplateColumns: 'minmax(0, 1fr) minmax(0, 1.4fr)' }}>
        <section className="card card-pad">
          <div className="stack stack-md" style={{ alignItems: 'center', textAlign: 'center' }}>
            {user?.avatar ? (
              <img className="avatar avatar-lg avatar-img" src={user.avatar} alt="Profile picture" />
            ) : (
              <div className="avatar avatar-lg">{getInitials(student?.name)}</div>
            )}
            <div>
              <div className="text-lg font-bold">{student?.name}</div>
              <div className="text-sm text-secondary">
                {student?.program} · Semester {student?.semester}
              </div>
            </div>
            <div className="full-width info-list" style={{ textAlign: 'left' }}>
              <div className="info-row">
                <span className="info-label row"><IdCard size={14} /> Student ID</span>
                <span className="info-value">{student?.studentId}</span>
              </div>
              <div className="info-row">
                <span className="info-label row"><Mail size={14} /> Email</span>
                <span className="info-value">{student?.email}</span>
              </div>
              <div className="info-row">
                <span className="info-label row"><GraduationCap size={14} /> Branch</span>
                <span className="info-value">{student?.branch}</span>
              </div>
              <div className="info-row">
                <span className="info-label row"><Phone size={14} /> Contact</span>
                <span className="info-value">{student?.contact}</span>
              </div>
              <div className="info-row">
                <span className="info-label">Member since</span>
                <span className="info-value">{formatDate(student.joinedDate)}</span>
              </div>
            </div>
            <p className="text-xs text-muted">
              Profile updates are handled by the librarian. Contact the library desk to
              change your details.
            </p>
          </div>
        </section>

        <div className="stack stack-md">
          <div className="grid grid-2">
            <StatCard icon={BookOpen} tone="blue" label="Books Borrowed" value={stats.borrowed + stats.returned} note={`${stats.borrowed} currently issued`} />
            <StatCard icon={RotateCcw} tone="green" label="Books Returned" value={stats.returned} note="Lifetime total" />
            <StatCard icon={IndianRupee} tone="red" label="Current Fine" value={`₹${stats.fine}`} note={stats.fine ? 'Pending payment' : 'No dues'} />
            <StatCard icon={Bookmark} tone="cyan" label="Active Reservations" value={reservations.length} note="In queue or ready" />
          </div>

          <section className="card">
            <div className="card-header"><h3>Current loans</h3></div>
            <div className="card-body">
              {active.length === 0 ? (
                <p className="text-secondary text-sm">No books issued right now.</p>
              ) : (
                active.map((tx) => {
                  const book = state.books.find((b) => b.id === tx.bookId)
                  return (
                    <div className="info-row" key={tx.id}>
                      <span className="info-label">{book?.title}</span>
                      <span className="info-value">Due {formatDate(tx.dueDate)}</span>
                    </div>
                  )
                })
              )}
              {fines.length > 0 && (
                <div className="info-row">
                  <span className="info-label text-danger">Outstanding fine</span>
                  <span className="info-value text-danger">₹{fines.reduce((s, f) => s + f.amount, 0)}</span>
                </div>
              )}
            </div>
          </section>
        </div>
      </div>
    </>
  )
}