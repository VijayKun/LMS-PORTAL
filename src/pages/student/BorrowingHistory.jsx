import { useMemo, useState } from 'react'
import { History, ArrowUpDown } from 'lucide-react'
import { useLibrary } from '../../store/LibraryContext.jsx'
import { useAuth } from '../../store/AuthContext.jsx'
import { usePageTitle } from '../../store/PageTitleContext.jsx'
import SearchBar from '../../components/ui/SearchBar.jsx'
import { Select } from '../../components/ui/Field.jsx'
import Badge from '../../components/ui/Badge.jsx'
import BookCover from '../../components/books/BookCover.jsx'
import EmptyState from '../../components/ui/EmptyState.jsx'
import { getBook, effectiveStatus, branchName } from '../../utils/selectors.js'
import { formatDate } from '../../utils/format.js'

export default function BorrowingHistory() {
  usePageTitle('Borrowing History')
  const { state } = useLibrary()
  const { user } = useAuth()

  const [query, setQuery] = useState('')
  const [status, setStatus] = useState('all')
  const [fine, setFine] = useState('all')
  const [sort, setSort] = useState({ key: 'issueDate', dir: 'desc' })

  const history = useMemo(
    () => state.transactions.filter((t) => t.studentId === user.id),
    [state.transactions, user.id],
  )

  const rows = useMemo(() => {
    const q = query.trim().toLowerCase()
    const filtered = history.filter((tx) => {
      const book = getBook(state, tx.bookId)
      if (q && !`${book.title} ${book.author} ${book.isbn}`.toLowerCase().includes(q)) return false
      if (status !== 'all' && effectiveStatus(tx) !== status) return false
      if (fine === 'with' && !tx.fine) return false
      if (fine === 'without' && tx.fine) return false
      return true
    })

    return filtered.sort((a, b) => {
      const key = sort.key
      let va = a[key] ?? ''
      let vb = b[key] ?? ''
      if (key === 'book') {
        va = getBook(state, a.bookId)?.title || ''
        vb = getBook(state, b.bookId)?.title || ''
      }
      if (va < vb) return sort.dir === 'asc' ? -1 : 1
      if (va > vb) return sort.dir === 'asc' ? 1 : -1
      return 0
    })
  }, [history, state, query, status, fine, sort])

  function toggleSort(key) {
    setSort((current) => ({
      key,
      dir: current.key === key && current.dir === 'asc' ? 'desc' : 'asc',
    }))
  }

  function SortHeader({ column, children }) {
    return (
      <th className="sortable" onClick={() => toggleSort(column)}>
        <span className="row" style={{ gap: 5, display: 'inline-flex' }}>
          {children}
          <ArrowUpDown size={11} className={sort.key === column ? '' : 'text-muted'} />
        </span>
      </th>
    )
  }

  const totalFine = history.reduce((sum, t) => sum + t.fine, 0)

  return (
    <>
      <div className="page-header">
        <div>
          <h1>Borrowing History</h1>
          <p className="page-sub">
            {history.length} record{history.length === 1 ? '' : 's'} · ₹{totalFine} in fines so far
          </p>
        </div>
      </div>

      <div className="card card-pad mb-4">
        <div className="filter-bar">
          <SearchBar
            value={query}
            onChange={setQuery}
            placeholder="Search by book title, author or ISBN..."
          />
          <Select value={status} onChange={(e) => setStatus(e.target.value)} aria-label="Status">
            <option value="all">All statuses</option>
            <option value="issued">Issued</option>
            <option value="overdue">Overdue</option>
            <option value="returned">Returned</option>
          </Select>
          <Select value={fine} onChange={(e) => setFine(e.target.value)} aria-label="Fine">
            <option value="all">With or without fine</option>
            <option value="with">With fine</option>
            <option value="without">No fine</option>
          </Select>
        </div>
      </div>

      <div className="card">
        <div className="card-body flush">
          {rows.length === 0 ? (
            <EmptyState
              icon={History}
              title="No records found"
              message="No borrowing records match the current search and filters."
            />
          ) : (
            <div className="table-responsive">
              <table className="table">
                <thead>
                  <tr>
                    <SortHeader column="book">Book</SortHeader>
                    <SortHeader column="issueDate">Issue Date</SortHeader>
                    <SortHeader column="returnDate">Return Date</SortHeader>
                    <SortHeader column="dueDate">Due Date</SortHeader>
                    <SortHeader column="fine">Fine</SortHeader>
                    <SortHeader column="status">Status</SortHeader>
                  </tr>
                </thead>
                <tbody>
                  {rows.map((tx) => {
                    const book = getBook(state, tx.bookId)
                    const statusKey = effectiveStatus(tx)
                    return (
                      <tr key={tx.id}>
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
                        <td>{formatDate(tx.issueDate)}</td>
                        <td>{tx.returnDate ? formatDate(tx.returnDate) : '—'}</td>
                        <td className="strong">{formatDate(tx.dueDate)}</td>
                        <td>{tx.fine > 0 ? <span className="text-danger font-semibold">₹{tx.fine}</span> : '—'}</td>
                        <td>
                          <Badge
                            status={
                              statusKey === 'returned' ? 'returned' : statusKey === 'overdue' ? 'overdue' : 'issued'
                            }
                          >
                            {statusKey === 'returned' ? 'Returned' : statusKey === 'overdue' ? 'Overdue' : 'Issued'}
                          </Badge>
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
    </>
  )
}
