import { useMemo, useState } from 'react'
import { BookOpen, SearchX, LayoutGrid, Rows3 } from 'lucide-react'
import { useLibrary } from '../../store/LibraryContext.jsx'
import { usePageTitle } from '../../store/PageTitleContext.jsx'
import SearchBar from '../../components/ui/SearchBar.jsx'
import { Select } from '../../components/ui/Field.jsx'
import BookCard from '../../components/books/BookCard.jsx'
import Pagination from '../../components/ui/Pagination.jsx'
import EmptyState from '../../components/ui/EmptyState.jsx'
import { availableCopiesOf } from '../../utils/selectors.js'
import { bookYearLabel } from '../../utils/format.js'
import { DEFAULT_PAGE_SIZE } from '../../data/constants.js'

const VIEW_KEY = 'lms.catalog.view'

export default function Catalog() {
  usePageTitle('Library Catalog')
  const { state } = useLibrary()

  const [query, setQuery] = useState('')
  const [category, setCategory] = useState('all')
  const [author, setAuthor] = useState('all')
  const [branch, setBranch] = useState('all')
  const [availability, setAvailability] = useState('all')
  const [year, setYear] = useState('all')
  const [page, setPage] = useState(1)
  const [view, setView] = useState(() => {
    try {
      return localStorage.getItem(VIEW_KEY) === 'shelf' ? 'shelf' : 'grid'
    } catch {
      return 'grid'
    }
  })

  function changeView(next) {
    setView(next)
    try {
      localStorage.setItem(VIEW_KEY, next)
    } catch {
      /* storage unavailable - preference applies for this session */
    }
  }

  const categories = useMemo(
    () => [...new Set(state.books.map((b) => b.category))].sort(),
    [state.books],
  )
  const authors = useMemo(
    () => [...new Set(state.books.map((b) => b.author))].sort(),
    [state.books],
  )
  const bookYears = [1, 2, 3, 4]

  const filtered = useMemo(() => {
    const q = query.trim().toLowerCase()
    return state.books.filter((book) => {
      const available = availableCopiesOf(state, book)
      if (q && !`${book.title} ${book.author} ${book.isbn} ${bookYearLabel(book.year)}`.toLowerCase().includes(q)) return false
      if (category !== 'all' && book.category !== category) return false
      if (author !== 'all' && book.author !== author) return false
      if (branch !== 'all' && book.branchId !== branch) return false
      // year filter: books tagged for this year, plus books meant for all years
      if (year !== 'all' && !(Number(book.year) === Number(year) || !Number(book.year))) return false
      if (availability === 'available' && available === 0) return false
      if (availability === 'unavailable' && available > 0) return false
      return true
    })
  }, [state, query, category, author, branch, availability, year])

  const totalPages = Math.max(1, Math.ceil(filtered.length / DEFAULT_PAGE_SIZE))
  const currentPage = Math.min(page, totalPages)
  const visible = filtered.slice(
    (currentPage - 1) * DEFAULT_PAGE_SIZE,
    currentPage * DEFAULT_PAGE_SIZE,
  )

  function resetPage() {
    setPage(1)
  }

  const hasFilters =
    query || category !== 'all' || author !== 'all' || branch !== 'all' || availability !== 'all' || year !== 'all'

  function clearFilters() {
    setQuery('')
    setCategory('all')
    setAuthor('all')
    setBranch('all')
    setAvailability('all')
    setYear('all')
    resetPage()
  }

  return (
    <>
      <div className="page-header">
        <div>
          <h1>Library Catalog</h1>
          <p className="page-sub">
            {filtered.length} title{filtered.length === 1 ? '' : 's'} found · {state.branches.length} branches
          </p>
        </div>
        <div className="page-actions">
          <div className="view-toggle" role="group" aria-label="Catalog view">
            <button
              type="button"
              className={view === 'grid' ? 'active' : ''}
              onClick={() => changeView('grid')}
              aria-label="Grid view"
              aria-pressed={view === 'grid'}
              data-tip="Grid view"
            >
              <LayoutGrid size={15} />
            </button>
            <button
              type="button"
              className={view === 'shelf' ? 'active' : ''}
              onClick={() => changeView('shelf')}
              aria-label="Shelf view"
              aria-pressed={view === 'shelf'}
              data-tip="Shelf view"
            >
              <Rows3 size={15} />
            </button>
          </div>
        </div>
      </div>

      <div className="card card-pad mb-4">
        <SearchBar
          value={query}
          onChange={(v) => {
            setQuery(v)
            resetPage()
          }}
          placeholder="Search by title, author, ISBN..."
        />
        <div className="filter-bar mt-3">
          <Select value={category} onChange={(e) => { setCategory(e.target.value); resetPage() }} aria-label="Category">
            <option value="all">All categories</option>
            {categories.map((c) => (
              <option key={c} value={c}>{c}</option>
            ))}
          </Select>

          <Select value={author} onChange={(e) => { setAuthor(e.target.value); resetPage() }} aria-label="Author">
            <option value="all">All authors</option>
            {authors.map((a) => (
              <option key={a} value={a}>{a}</option>
            ))}
          </Select>

          <Select value={branch} onChange={(e) => { setBranch(e.target.value); resetPage() }} aria-label="Branch">
            <option value="all">All branches</option>
            {state.branches.map((b) => (
              <option key={b.id} value={b.id}>{b.name}</option>
            ))}
          </Select>

          <Select value={availability} onChange={(e) => { setAvailability(e.target.value); resetPage() }} aria-label="Availability">
            <option value="all">Any availability</option>
            <option value="available">Available now</option>
            <option value="unavailable">All copies issued</option>
          </Select>

          <Select value={year} onChange={(e) => { setYear(e.target.value); resetPage() }} aria-label="Book year">
            <option value="all">All years</option>
            {bookYears.map((y) => (
              <option key={y} value={y}>{bookYearLabel(y)}</option>
            ))}
          </Select>

          {hasFilters && (
            <button className="btn btn-ghost btn-sm" onClick={clearFilters}>
              Clear filters
            </button>
          )}
        </div>
      </div>

      {visible.length === 0 ? (
        <div className="card">
          <EmptyState
            kind="search"
            icon={SearchX}
            title="No books match your search"
            message="Try a different keyword or clear the filters to see the full catalog."
            action={
              <button className="btn btn-primary" onClick={clearFilters}>
                Clear filters
              </button>
            }
          />
        </div>
      ) : (
        <>
          {view === 'shelf' ? (
            <div className="book-shelf">
              {visible.map((book) => (
                <BookCard key={book.id} book={book} layout="row" />
              ))}
            </div>
          ) : (
            <div className="book-grid">
              {visible.map((book) => (
                <BookCard key={book.id} book={book} />
              ))}
            </div>
          )}
          <Pagination
            page={currentPage}
            totalPages={totalPages}
            totalItems={filtered.length}
            pageSize={DEFAULT_PAGE_SIZE}
            onPage={setPage}
          />
        </>
      )}
    </>
  )
}
