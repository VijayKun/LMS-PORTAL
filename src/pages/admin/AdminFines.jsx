import { useMemo, useState } from 'react'
import { IndianRupee, Receipt, AlertTriangle, Eye, CheckCircle2 } from 'lucide-react'
import { useLibrary } from '../../store/LibraryContext.jsx'
import { usePageTitle } from '../../store/PageTitleContext.jsx'
import { useToast } from '../../store/ToastContext.jsx'
import SearchBar from '../../components/ui/SearchBar.jsx'
import { Select } from '../../components/ui/Field.jsx'
import Badge from '../../components/ui/Badge.jsx'
import StatCard from '../../components/ui/StatCard.jsx'
import EmptyState from '../../components/ui/EmptyState.jsx'
import Modal from '../../components/ui/Modal.jsx'
import {
  getStudent,
  getBook,
  overdueLoans,
  liveFine,
  daysRemaining,
} from '../../utils/selectors.js'
import { formatDate, formatNumber } from '../../utils/format.js'
import { simulatedLatency } from '../../services/api.js'

export default function AdminFines() {
  usePageTitle('Fine Management')
  const { state, payFine, track } = useLibrary()
  const { toast } = useToast()

  const [query, setQuery] = useState('')
  const [status, setStatus] = useState('all')
  const [viewing, setViewing] = useState(null)

  /** Settled fines + live fines still accruing on overdue books. */
  const liveRows = useMemo(
    () =>
      overdueLoans(state).map((tx) => ({
        id: `live-${tx.id}`,
        studentId: tx.studentId,
        bookId: tx.bookId,
        dueDate: tx.dueDate,
        returnDate: null,
        overdueDays: -daysRemaining(tx.dueDate),
        amount: liveFine(tx),
        status: 'pending',
        live: true,
        txId: tx.id,
      })),
    [state],
  )

  const allRows = [...liveRows, ...state.fines.map((f) => ({ ...f, live: false }))]

  const totals = {
    outstanding: allRows.filter((r) => r.status === 'pending').reduce((s, r) => s + r.amount, 0),
    collected: state.fines.filter((f) => f.status === 'paid').reduce((s, f) => s + f.amount, 0),
    pendingCount: allRows.filter((r) => r.status === 'pending').length,
    overdueBooks: liveRows.length,
  }

  const rows = useMemo(() => {
    const q = query.trim().toLowerCase()
    return allRows.filter((r) => {
      const student = getStudent(state, r.studentId)
      const book = getBook(state, r.bookId)
      if (status !== 'all' && r.status !== status) return false
      if (q && !`${student?.name} ${student?.studentId} ${book?.title}`.toLowerCase().includes(q)) return false
      return true
    })
  }, [allRows, state, query, status])

  function recordPayment(fine) {
    if (fine.live) {
      toast('Return the overdue book first — the fine is still accruing.', 'info')
      return
    }
    payFine(fine.id) // optimistic
    track(simulatedLatency(350))
    toast(`Payment of ₹${fine.amount} recorded successfully.`, 'success')
  }

  return (
    <>
      <div className="page-header">
        <div>
          <h1>Fine Management</h1>
          <p className="page-sub">Track late fees, record counter payments and follow up on dues.</p>
        </div>
      </div>

      <div className="grid grid-4 mb-4">
        <StatCard icon={IndianRupee} tone="red" label="Total Outstanding" value={`₹${formatNumber(totals.outstanding)}`} note={`${totals.pendingCount} pending record(s)`} />
        <StatCard icon={Receipt} tone="green" label="Total Collected" value={`₹${formatNumber(totals.collected)}`} note="Paid at the counter" />
        <StatCard icon={IndianRupee} tone="amber" label="Pending Fines" value={`₹${formatNumber(totals.outstanding)}`} note="Includes live overdue fees" />
        <StatCard icon={AlertTriangle} tone="blue" label="Overdue Books" value={totals.overdueBooks} note="Fines still accruing" />
      </div>

      <div className="card card-pad mb-4">
        <div className="filter-bar">
          <SearchBar value={query} onChange={setQuery} placeholder="Search by student or book..." />
          <Select value={status} onChange={(e) => setStatus(e.target.value)} aria-label="Payment status">
            <option value="all">All payments</option>
            <option value="pending">Pending</option>
            <option value="paid">Paid</option>
          </Select>
        </div>
      </div>

      <div className="card">
        <div className="card-body flush">
          {rows.length === 0 ? (
            <EmptyState icon={IndianRupee} title="No fine records" message="No fines match the current filters." />
          ) : (
            <div className="table-responsive">
              <table className="table">
                <thead>
                  <tr>
                    <th>Student</th>
                    <th>Book</th>
                    <th>Overdue Days</th>
                    <th>Fine</th>
                    <th>Payment Status</th>
                    <th className="text-right">Action</th>
                  </tr>
                </thead>
                <tbody>
                  {rows.map((f) => {
                    const student = getStudent(state, f.studentId)
                    const book = getBook(state, f.bookId)
                    return (
                      <tr key={f.id}>
                        <td>
                          <div className="book-title">{student?.name}</div>
                          <div className="book-meta">{student?.studentId}</div>
                        </td>
                        <td>
                          <div className="book-title">{book?.title}</div>
                          <div className="book-meta">Due {formatDate(f.dueDate)}</div>
                        </td>
                        <td className="strong">{f.overdueDays}</td>
                        <td className="font-bold text-danger">₹{f.amount}</td>
                        <td>
                          <Badge status={f.status === 'paid' ? 'paid' : 'pending'}>
                            {f.status === 'paid' ? 'Paid' : f.live ? 'Accruing' : 'Pending'}
                          </Badge>
                        </td>
                        <td>
                          <div className="actions-cell">
                            <button className="icon-btn" title="View details" onClick={() => setViewing(f)} aria-label="View fine details">
                              <Eye size={15} />
                            </button>
                            <button
                              className="btn btn-primary btn-sm"
                              onClick={() => recordPayment(f)}
                              disabled={f.status === 'paid'}
                            >
                              <CheckCircle2 size={13} />
                              {f.status === 'paid' ? 'Paid' : 'Record Payment'}
                            </button>
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

      <Modal
        open={Boolean(viewing)}
        title="Fine Details"
        onClose={() => setViewing(null)}
        footer={<button className="btn btn-secondary" onClick={() => setViewing(null)}>Close</button>}
      >
        {viewing && (
          <div className="info-list">
            <div className="info-row"><span className="info-label">Student</span><span className="info-value">{getStudent(state, viewing.studentId)?.name}</span></div>
            <div className="info-row"><span className="info-label">Book</span><span className="info-value">{getBook(state, viewing.bookId)?.title}</span></div>
            <div className="info-row"><span className="info-label">Due date</span><span className="info-value">{formatDate(viewing.dueDate)}</span></div>
            <div className="info-row"><span className="info-label">Return date</span><span className="info-value">{viewing.returnDate ? formatDate(viewing.returnDate) : 'Not returned'}</span></div>
            <div className="info-row"><span className="info-label">Overdue days</span><span className="info-value">{viewing.overdueDays}</span></div>
            <div className="info-row"><span className="info-label">Amount</span><span className="info-value text-danger">₹{viewing.amount}</span></div>
            <div className="info-row"><span className="info-label">Status</span><span className="info-value"><Badge status={viewing.status}>{viewing.status}</Badge></span></div>
            {viewing.paidDate && (
              <div className="info-row"><span className="info-label">Paid on</span><span className="info-value">{formatDate(viewing.paidDate)}</span></div>
            )}
          </div>
        )}
      </Modal>
    </>
  )
}
