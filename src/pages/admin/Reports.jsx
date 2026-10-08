import { useState } from 'react'
import {
  FileText,
  Download,
  Printer,
  Clock,
  IndianRupee,
  History,
  BookOpen,
  Building2,
} from 'lucide-react'
import { usePageTitle } from '../../store/PageTitleContext.jsx'
import { useToast } from '../../store/ToastContext.jsx'
import { useLibrary } from '../../store/LibraryContext.jsx'
import { Field, Input, Select } from '../../components/ui/Field.jsx'
import Badge from '../../components/ui/Badge.jsx'
import EmptyState from '../../components/ui/EmptyState.jsx'
import LoadingState from '../../components/ui/LoadingState.jsx'
import { generateReportRequest } from '../../services/api.js'
import { getStudent, getBook, branchStats, overdueLoans, daysRemaining, liveFine, effectiveStatus } from '../../utils/selectors.js'
import { formatDate, todayISO, addDays } from '../../utils/format.js'
import { toCsv, downloadTextFile } from '../../utils/csv.js'

const REPORTS = [
  { id: 'overdue', label: 'Overdue Books', icon: Clock, description: 'Loans that are past their due date.' },
  { id: 'fines', label: 'Fine Records', icon: IndianRupee, description: 'All fine transactions and payment status.' },
  { id: 'history', label: 'Borrowing History', icon: History, description: 'Every issue and return in a period.' },
  { id: 'availability', label: 'Book Availability', icon: BookOpen, description: 'Copies held vs available per title.' },
  { id: 'branches', label: 'Branch-wise Borrowing', icon: Building2, description: 'Activity summary for each branch.' },
]

export default function Reports() {
  usePageTitle('Reports')
  const { state } = useLibrary()
  const { toast } = useToast()

  const [type, setType] = useState('overdue')
  const [from, setFrom] = useState(addDays(todayISO(), -90))
  const [to, setTo] = useState(todayISO())
  const [branch, setBranch] = useState('all')
  const [status, setStatus] = useState('all')
  const [generating, setGenerating] = useState(false)
  const [report, setReport] = useState(null)

  const activeReport = REPORTS.find((r) => r.id === type)

  function buildRows() {
    const inRange = (date) => (!date ? false : date >= from && date <= to)

    if (type === 'overdue') {
      const headers = ['Student', 'Book', 'Due Date', 'Days Overdue', 'Fine']
      const rows = overdueLoans(state)
        .filter((tx) => inRange(tx.dueDate))
        .map((tx) => {
          const book = getBook(state, tx.bookId)
          if (branch !== 'all' && book?.branchId !== branch) return null
          return [
            getStudent(state, tx.studentId)?.name,
            book?.title,
            formatDate(tx.dueDate),
            Math.abs(daysRemaining(tx.dueDate)),
            `₹${liveFine(tx)}`,
          ]
        })
        .filter(Boolean)
      return { headers, rows }
    }

    if (type === 'fines') {
      const headers = ['Student', 'Book', 'Overdue Days', 'Amount', 'Status']
      const rows = state.fines
        .filter((f) => inRange(f.returnDate))
        .filter((f) => status === 'all' || f.status === status)
        .map((f) => {
          const book = getBook(state, f.bookId)
          if (branch !== 'all' && book?.branchId !== branch) return null
          return [
            getStudent(state, f.studentId)?.name,
            book?.title,
            f.overdueDays,
            `₹${f.amount}`,
            f.status === 'paid' ? 'Paid' : 'Pending',
          ]
        })
        .filter(Boolean)
      return { headers, rows }
    }

    if (type === 'history') {
      const headers = ['Student', 'Book', 'Issue Date', 'Return Date', 'Fine', 'Status']
      const rows = state.transactions
        .filter((tx) => inRange(tx.issueDate))
        .map((tx) => {
          const book = getBook(state, tx.bookId)
          if (branch !== 'all' && book?.branchId !== branch) return null
          if (status !== 'all' && effectiveStatus(tx) !== status) return null
          return [
            getStudent(state, tx.studentId)?.name,
            book?.title,
            formatDate(tx.issueDate),
            tx.returnDate ? formatDate(tx.returnDate) : '—',
            tx.fine ? `₹${tx.fine}` : '—',
            effectiveStatus(tx) === 'returned' ? 'Returned' : effectiveStatus(tx) === 'overdue' ? 'Overdue' : 'Issued',
          ]
        })
        .filter(Boolean)
      return { headers, rows }
    }

    if (type === 'availability') {
      const headers = ['Title', 'Author', 'Branch', 'Total Copies', 'Available']
      const rows = state.books
        .filter((b) => branch === 'all' || b.branchId === branch)
        .map((b) => [
          b.title,
          b.author,
          state.branches.find((x) => x.id === b.branchId)?.name || '—',
          b.totalCopies,
          b.availableCopies,
        ])
      return { headers, rows }
    }

    // branch-wise
    const headers = ['Branch', 'Titles', 'Total Copies', 'Available', 'Issued', 'Transactions']
    const rows = branchStats(state)
      .filter((b) => branch === 'all' || b.id === branch)
      .map((b) => [b.name, b.titles, b.totalCopies, b.availableCopies, b.issuedCopies, b.transactions])
    return { headers, rows }
  }

  /** Mock generation - replaced by GET /api/reports/:type later. */
  async function generate(e) {
    e.preventDefault()
    if (from > to) {
      toast('The start date must be before the end date.', 'error')
      return
    }
    setGenerating(true)
    setReport(null)
    await generateReportRequest()
    const data = buildRows()
    setReport(data)
    setGenerating(false)
    toast('Report generated successfully.', 'success')
  }

  function exportCsv() {
    if (!report) return
    const csv = toCsv(report.headers, report.rows)
    const slug = activeReport.label.toLowerCase().replace(/[^a-z0-9]+/g, '-')
    downloadTextFile(`${slug}_${from}_to_${to}.csv`, csv)
    toast(`Exported ${report.rows.length} row(s) to CSV.`, 'success')
  }

  function printReport() {
    if (!report) return
    window.print()
  }

  return (
    <>
      <div className="page-header no-print">
        <div>
          <h1>Reports</h1>
          <p className="page-sub">Generate, preview and export library records.</p>
        </div>
      </div>

      <div className="grid grid-split" style={{ gridTemplateColumns: 'minmax(0, 320px) minmax(0, 1fr)' }}>
        <form className="card card-pad no-print" onSubmit={generate}>
          <div className="section-title mb-3">Report type</div>
          <div className="stack stack-sm mb-4">
            {REPORTS.map((r) => (
              <button
                type="button"
                key={r.id}
                onClick={() => { setType(r.id); setReport(null) }}
                className="report-option"
                style={{
                  border: `1.5px solid ${type === r.id ? 'var(--primary)' : 'var(--border-strong)'}`,
                  background: type === r.id ? 'var(--primary-soft)' : '#fff',
                  color: type === r.id ? 'var(--primary-dark)' : 'var(--text)',
                }}
              >
                <r.icon size={16} />
                <span style={{ textAlign: 'left' }}>
                  <span className="font-semibold" style={{ display: 'block', fontSize: 13.5 }}>{r.label}</span>
                  <span className="text-xs" style={{ color: 'var(--text-muted)', fontWeight: 400 }}>{r.description}</span>
                </span>
              </button>
            ))}
          </div>

          <Field label="From date" htmlFor="r-from">
            <Input id="r-from" type="date" value={from} onChange={(e) => setFrom(e.target.value)} />
          </Field>
          <Field label="To date" htmlFor="r-to">
            <Input id="r-to" type="date" value={to} onChange={(e) => setTo(e.target.value)} />
          </Field>
          <Field label="Branch">
            <Select value={branch} onChange={(e) => setBranch(e.target.value)}>
              <option value="all">All branches</option>
              {state.branches.map((b) => <option key={b.id} value={b.id}>{b.name}</option>)}
            </Select>
          </Field>
          {type !== 'availability' && type !== 'branches' && (
            <Field label="Status">
              <Select value={status} onChange={(e) => setStatus(e.target.value)}>
                <option value="all">All statuses</option>
                <option value="issued">Issued</option>
                <option value="overdue">Overdue</option>
                <option value="returned">Returned</option>
                <option value="pending">Pending</option>
                <option value="paid">Paid</option>
              </Select>
            </Field>
          )}

          <button className="btn btn-primary btn-block" type="submit" disabled={generating}>
            <FileText size={16} /> {generating ? 'Generating...' : 'Generate Report'}
          </button>
        </form>

        <div className="stack stack-md">
          <div className="card">
            <div className="print-only print-report-head">
              <strong>Library Report: {activeReport.label}</strong>
              <span> · {formatDate(from)} → {formatDate(to)} · {branch === 'all' ? 'All branches' : state.branches.find((b) => b.id === branch)?.name}</span>
            </div>
            <div className="card-header">
              <h3>{activeReport.label} Preview</h3>
              <div className="card-actions no-print">
                {report && (
                  <>
                    <Badge tone="gray">{report.rows.length} row(s)</Badge>
                    <button className="btn btn-secondary btn-sm" onClick={exportCsv}>
                      <Download size={14} /> Export CSV
                    </button>
                    <button className="btn btn-secondary btn-sm" onClick={printReport}>
                      <Printer size={14} /> Print
                    </button>
                  </>
                )}
              </div>
            </div>
            <div className="card-body flush">
              {generating ? (
                <LoadingState label="Generating your report..." />
              ) : !report ? (
                <EmptyState
                  icon={FileText}
                  title="No report generated yet"
                  message={`Choose a date range and click "Generate Report" to preview the ${activeReport.label.toLowerCase()}.`}
                />
              ) : report.rows.length === 0 ? (
                <EmptyState
                  icon={FileText}
                  title="No records in this range"
                  message="Try widening the date range or clearing the filters."
                />
              ) : (
                <div className="table-responsive import-preview-scroll">
                  <table className="table">
                    <thead>
                      <tr>{report.headers.map((h) => <th key={h}>{h}</th>)}</tr>
                    </thead>
                    <tbody>
                      {report.rows.map((row, index) => (
                        <tr key={index}>
                          {row.map((cell, cellIndex) => (
                            <td key={cellIndex} className={cellIndex === 0 ? 'strong' : ''}>{cell}</td>
                          ))}
                        </tr>
                      ))}
                    </tbody>
                  </table>
                </div>
              )}
            </div>
          </div>

          <div className="card card-pad text-sm text-secondary">
            <strong>Reporting period:</strong> {formatDate(from)} → {formatDate(to)} ·{' '}
            {branch === 'all' ? 'All branches' : state.branches.find((b) => b.id === branch)?.name}
          </div>
        </div>
      </div>
    </>
  )
}
