import { IndianRupee, Receipt } from 'lucide-react'
import { useLibrary } from '../../store/LibraryContext.jsx'
import { useAuth } from '../../store/AuthContext.jsx'
import { usePageTitle } from '../../store/PageTitleContext.jsx'
import { useToast } from '../../store/ToastContext.jsx'
import Badge from '../../components/ui/Badge.jsx'
import BookCover from '../../components/books/BookCover.jsx'
import EmptyState from '../../components/ui/EmptyState.jsx'
import StatCard from '../../components/ui/StatCard.jsx'
import {
  getBook,
  outstandingFines,
  totalOf,
  activeLoansOf,
  isOverdue,
} from '../../utils/selectors.js'
import { formatDate } from '../../utils/format.js'
import { FINE_PER_DAY_LABEL } from '../../data/constants.js'

export default function StudentFines() {
  usePageTitle('Fines')
  const { state } = useLibrary()
  const { user } = useAuth()
  const { toast } = useToast()

  const pending = outstandingFines(state, user.id)
  const paid = state.fines.filter((f) => f.studentId === user.id && f.status === 'paid')
  const overdueLoans = activeLoansOf(state, user.id).filter(isOverdue)

  const outstanding = totalOf(pending)
  const cleared = totalOf(paid)

  const records = [
    ...pending.map((f) => ({ ...f, live: Boolean(f.live) })),
    ...paid.map((f) => ({ ...f, paid: true })),
  ]

  return (
    <>
      <div className="page-header">
        <div>
          <h1>Fines</h1>
          <p className="page-sub">
            Late returns are charged at {FINE_PER_DAY_LABEL}. Pay at the library counter.
          </p>
        </div>
      </div>

      <div className="grid grid-3 mb-4 fines-stats">
        <StatCard
          icon={IndianRupee}
          tone="red"
          label="Outstanding Fine"
          value={`₹${outstanding}`}
          note={outstanding ? 'Payment pending' : 'No dues pending'}
        />
        <StatCard icon={Receipt} tone="green" label="Paid to Date" value={`₹${cleared}`} note={`${paid.length} payment(s)`} />
        <StatCard
          icon={IndianRupee}
          tone="amber"
          label="Currently Overdue"
          value={`${overdueLoans.length} Book${overdueLoans.length === 1 ? '' : 's'}`}
          note="Fines keep growing daily"
        />
      </div>

      {outstanding > 0 && (
        <div className="card card-pad mb-4" style={{ borderLeft: '4px solid var(--danger)' }}>
          <div className="row-between wrap fine-alert">
            <div>
              <div className="font-semibold">You have ₹{outstanding} in pending fines</div>
              <div className="text-sm text-secondary mt-2">
                Clear your dues at the Main Library counter to borrow new books without interruption.
              </div>
            </div>
            <button
              className="btn btn-primary"
              onClick={() => toast('Online payment is not enabled in this demo. Please pay at the library counter.', 'info')}
            >
              Pay ₹{outstanding}
            </button>
          </div>
        </div>
      )}

      <div className="card">
        <div className="card-header">
          <h3>Fine Records</h3>
          <div className="card-actions">
            <Badge tone="gray">{records.length} record(s)</Badge>
          </div>
        </div>
        <div className="card-body flush">
          {records.length === 0 ? (
            <EmptyState
              icon={IndianRupee}
              title="No fines"
              message="You have never been charged a late fee. Keep it up!"
            />
          ) : (
            <>
              <div className="table-responsive hide-mobile">
              <table className="table">
                <thead>
                  <tr>
                    <th>Book</th>
                    <th>Due Date</th>
                    <th>Returned</th>
                    <th>Overdue Days</th>
                    <th>Fine</th>
                    <th>Status</th>
                    <th className="text-right">Action</th>
                  </tr>
                </thead>
                <tbody>
                  {records.map((f) => {
                    const book = getBook(state, f.bookId)
                    return (
                      <tr key={f.id}>
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
                        <td>{formatDate(f.dueDate)}</td>
                        <td>{f.returnDate ? formatDate(f.returnDate) : 'Not returned'}</td>
                        <td className="strong">{f.overdueDays}</td>
                        <td className="font-bold text-danger">₹{f.amount}</td>
                        <td>
                          <Badge status={f.status === 'paid' ? 'paid' : 'pending'}>
                            {f.status === 'paid' ? 'Paid' : f.live ? 'Accruing' : 'Pending'}
                          </Badge>
                        </td>
                        <td className="text-right">
                          {f.status === 'paid' ? (
                            <span className="text-sm text-muted">Paid on {formatDate(f.paidDate)}</span>
                          ) : (
                            <button
                              className="btn btn-secondary btn-sm"
                              onClick={() =>
                                toast(
                                  f.live
                                    ? 'Return the book first to settle this fine (demo).'
                                    : 'Payable at the library counter (demo).',
                                  'info',
                                )
                              }
                            >
                              Pay Now
                            </button>
                          )}
                        </td>
                      </tr>
                    )
                  })}
                </tbody>
              </table>
              </div>

              {/* Phones: one compact card per fine record */}
              <div className="mrec-list only-mobile">
                {records.map((f) => {
                  const book = getBook(state, f.bookId)
                  return (
                    <div className="mrec-card" key={f.id}>
                      <div className="mrec-top">
                        <div className="mrec-cover">
                          <BookCover book={book} size="sm" />
                        </div>
                        <div className="mrec-book">
                          <div className="mrec-title">{book.title}</div>
                          <div className="mrec-meta">{book.author}</div>
                        </div>
                      </div>
                      <div className="info-list compact mrec-rows">
                        <div className="info-row">
                          <span className="info-label">Due</span>
                          <span className="info-value">{formatDate(f.dueDate)}</span>
                        </div>
                        <div className="info-row">
                          <span className="info-label">Returned</span>
                          <span className="info-value">{f.returnDate ? formatDate(f.returnDate) : 'Not returned'}</span>
                        </div>
                        <div className="info-row">
                          <span className="info-label">Overdue</span>
                          <span className="info-value">
                            {f.overdueDays} day{f.overdueDays === 1 ? '' : 's'}
                          </span>
                        </div>
                        <div className="info-row">
                          <span className="info-label">Fine</span>
                          <span className="info-value text-danger font-bold">₹{f.amount}</span>
                        </div>
                        <div className="info-row">
                          <span className="info-label">Status</span>
                          <span className="info-value">
                            <Badge status={f.status === 'paid' ? 'paid' : 'pending'}>
                              {f.status === 'paid' ? 'Paid' : f.live ? 'Accruing' : 'Pending'}
                            </Badge>
                          </span>
                        </div>
                        {f.status === 'paid' && (
                          <div className="info-row">
                            <span className="info-label">Paid on</span>
                            <span className="info-value">{formatDate(f.paidDate)}</span>
                          </div>
                        )}
                      </div>
                      {f.status !== 'paid' && (
                        <div className="mrec-actions">
                          <button
                            className="btn btn-secondary btn-sm"
                            onClick={() =>
                              toast(
                                f.live
                                  ? 'Return the book first to settle this fine (demo).'
                                  : 'Payable at the library counter (demo).',
                                'info',
                              )
                            }
                          >
                            Pay Now
                          </button>
                        </div>
                      )}
                    </div>
                  )
                })}
              </div>
            </>
          )}
        </div>
      </div>
    </>
  )
}
