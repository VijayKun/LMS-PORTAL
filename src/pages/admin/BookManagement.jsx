import { useMemo, useState } from 'react'
import { Link, useNavigate } from 'react-router-dom'
import { Plus, Upload, Eye, Pencil, Trash2, BookOpen, SearchX } from 'lucide-react'
import { useLibrary } from '../../store/LibraryContext.jsx'
import { usePageTitle } from '../../store/PageTitleContext.jsx'
import { useToast } from '../../store/ToastContext.jsx'
import SearchBar from '../../components/ui/SearchBar.jsx'
import { Select } from '../../components/ui/Field.jsx'
import Badge from '../../components/ui/Badge.jsx'
import BookCover from '../../components/books/BookCover.jsx'
import Pagination from '../../components/ui/Pagination.jsx'
import EmptyState from '../../components/ui/EmptyState.jsx'
import { bookYearLabel } from '../../utils/format.js'
import Modal from '../../components/ui/Modal.jsx'
import ConfirmDialog from '../../components/ui/ConfirmDialog.jsx'
import { availableCopiesOf, branchName } from '../../utils/selectors.js'
import { formatDate } from '../../utils/format.js'
import { DEFAULT_PAGE_SIZE } from '../../data/constants.js'

export default function BookManagement() {
  usePageTitle('Book Management')
  const { state, deleteBook } = useLibrary()
  const { toast } = useToast()
  const navigate = useNavigate()

  const [query, setQuery] = useState('')
  const [category, setCategory] = useState('all')
  const [branch, setBranch] = useState('all')
  const [availability, setAvailability] = useState('all')
  const [author, setAuthor] = useState('all')
  const [year, setYear] = useState('all')
  const [page, setPage] = useState(1)
  const [viewing, setViewing] = useState(null)
  const [deleting, setDeleting] = useState(null)

  const categories = useMemo(() => [...new Set(state.books.map((b) => b.category))].sort(), [state.books])
  const authors = useMemo(() => [...new Set(state.books.map((b) => b.author))].sort(), [state.books])

  const filtered = useMemo(() => {
    const q = query.trim().toLowerCase()
    return state.books.filter((book) => {
      const available = availableCopiesOf(state, book)
      if (q && !`${book.title} ${book.author} ${book.isbn} ${bookYearLabel(book.year)}`.toLowerCase().includes(q)) return false
      if (category !== 'all' && book.category !== category) return false
      if (author !== 'all' && book.author !== author) return false
      if (branch !== 'all' && book.branchId !== branch) return false
      if (year !== 'all' && !(Number(book.year) === Number(year) || !Number(book.year))) return false
      if (availability === 'available' && available === 0) return false
      if (availability === 'unavailable' && available > 0) return false
      return true
    })
  }, [state, query, category, author, branch, year, availability])

  const totalPages = Math.max(1, Math.ceil(filtered.length / DEFAULT_PAGE_SIZE))
  const currentPage = Math.min(page, totalPages)
  const rows = filtered.slice((currentPage - 1) * DEFAULT_PAGE_SIZE, currentPage * DEFAULT_PAGE_SIZE)

  function confirmDelete() {
    deleteBook(deleting.id)
    toast(`"${deleting.title}" deleted successfully.`, 'success')
    setDeleting(null)
  }

  function clearFilters() {
    setQuery('')
    setCategory('all')
    setBranch('all')
    setAvailability('all')
    setAuthor('all')
    setYear('all')
    setPage(1)
  }

  return (
    <>
      <div className="page-header">
        <div>
          <h1>Book Management</h1>
          <p className="page-sub">{state.books.length} titles · {state.books.reduce((s, b) => s + b.totalCopies, 0)} copies in the catalog</p>
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

      <div className="card card-pad mb-4">
        <SearchBar value={query} onChange={(v) => { setQuery(v); setPage(1) }} placeholder="Search books..." />
        <div className="filter-bar mt-3">
          <Select value={category} onChange={(e) => { setCategory(e.target.value); setPage(1) }} aria-label="Category">
            <option value="all">All categories</option>
            {categories.map((c) => <option key={c} value={c}>{c}</option>)}
          </Select>
          <Select value={author} onChange={(e) => { setAuthor(e.target.value); setPage(1) }} aria-label="Author">
            <option value="all">All authors</option>
            {authors.map((a) => <option key={a} value={a}>{a}</option>)}
          </Select>
          <Select value={branch} onChange={(e) => { setBranch(e.target.value); setPage(1) }} aria-label="Branch">
            <option value="all">All branches</option>
            {state.branches.map((b) => <option key={b.id} value={b.id}>{b.name}</option>)}
          </Select>
          <Select value={availability} onChange={(e) => { setAvailability(e.target.value); setPage(1) }} aria-label="Availability">
            <option value="all">Any availability</option>
            <option value="available">Available</option>
            <option value="unavailable">Issued out</option>
          </Select>
          <Select value={year} onChange={(e) => { setYear(e.target.value); setPage(1) }} aria-label="Book year">
            <option value="all">All years</option>
            {[1, 2, 3, 4].map((y) => <option key={y} value={y}>{bookYearLabel(y)}</option>)}
          </Select>
        </div>
      </div>

      <div className="card">
        <div className="card-body flush">
          {rows.length === 0 ? (
            <EmptyState
              icon={SearchX}
              title="No books found"
              message="No titles match the current search and filters."
              action={<button className="btn btn-primary" onClick={clearFilters}>Clear filters</button>}
            />
          ) : (
            <div className="table-responsive">
              <table className="table">
                <thead>
                  <tr>
                    <th>Cover</th>
                    <th>Title</th>
                    <th>ISBN</th>
                    <th>Category</th>
                    <th>Copies</th>
                    <th>Available</th>
                    <th>Branch</th>
                    <th>Status</th>
                    <th className="text-right">Actions</th>
                  </tr>
                </thead>
                <tbody>
                  {rows.map((book) => {
                    const available = availableCopiesOf(state, book)
                    return (
                      <tr key={book.id}>
                        <td><div className="cover-sm-row"><BookCover book={book} size="sm" /></div></td>
                        <td>
                          <div className="book-title">{book.title}</div>
                          <div className="book-meta">{book.author}</div>
                        </td>
                        <td className="text-sm">{book.isbn}</td>
                        <td>{book.category}</td>
                        <td>{book.totalCopies}</td>
                        <td><Badge status={available > 0 ? 'available' : 'expired'}>{available}</Badge></td>
                        <td>{branchName(state, book.branchId)}</td>
                        <td>
                          <Badge status={available > 0 ? 'available' : 'expired'}>
                            {available > 0 ? 'Available' : 'Issued out'}
                          </Badge>
                        </td>
                        <td>
                          <div className="actions-cell">
                            <button className="icon-btn" title="View" onClick={() => setViewing(book)} aria-label={`View ${book.title}`}>
                              <Eye size={15} />
                            </button>
                            <button
                              className="icon-btn"
                              title="Edit"
                              onClick={() => navigate(`/admin/books/${book.id}/edit`)}
                              aria-label={`Edit ${book.title}`}
                            >
                              <Pencil size={15} />
                            </button>
                            <button
                              className="icon-btn"
                              title="Delete"
                              onClick={() => setDeleting(book)}
                              aria-label={`Delete ${book.title}`}
                              style={{ color: 'var(--danger)' }}
                            >
                              <Trash2 size={15} />
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

      <Pagination page={currentPage} totalPages={totalPages} totalItems={filtered.length} pageSize={DEFAULT_PAGE_SIZE} onPage={setPage} />

      <Modal open={Boolean(viewing)} title={viewing?.title || 'Book details'} size="lg" onClose={() => setViewing(null)}
        footer={
          <>
            <button className="btn btn-secondary" onClick={() => setViewing(null)}>Close</button>
            <button className="btn btn-primary" onClick={() => { navigate(`/admin/books/${viewing.id}/edit`) }}>
              <Pencil size={15} /> Edit Book
            </button>
          </>
        }
      >
        {viewing && (
          <div className="grid grid-split" style={{ gridTemplateColumns: '120px minmax(0, 1fr)', gap: 20 }}>
            <div><BookCover book={viewing} /></div>
            <div className="info-list">
              <div className="info-row"><span className="info-label">Author</span><span className="info-value">{viewing.author}</span></div>
              <div className="info-row"><span className="info-label">ISBN</span><span className="info-value">{viewing.isbn}</span></div>
              <div className="info-row"><span className="info-label">Publisher</span><span className="info-value">{viewing.publisher}</span></div>
              <div className="info-row"><span className="info-label">Published</span><span className="info-value">{formatDate(viewing.publicationDate)}</span></div>
              <div className="info-row"><span className="info-label">Category</span><span className="info-value">{viewing.category}</span></div>
              <div className="info-row"><span className="info-label">Branch</span><span className="info-value">{branchName(state, viewing.branchId)}</span></div>
              <div className="info-row"><span className="info-label">Copies</span><span className="info-value">{availableCopiesOf(state, viewing)} / {viewing.totalCopies} available</span></div>
            </div>
            <p className="text-secondary" style={{ gridColumn: '1 / -1' }}>{viewing.description}</p>
          </div>
        )}
      </Modal>

      <ConfirmDialog
        open={Boolean(deleting)}
        title="Delete this book?"
        confirmLabel="Delete Book"
        message={deleting ? `"${deleting.title}" and all of its loans, reservations and fine records will be removed. This cannot be undone.` : ''}
        onClose={() => setDeleting(null)}
        onConfirm={confirmDelete}
      />
    </>
  )
}
