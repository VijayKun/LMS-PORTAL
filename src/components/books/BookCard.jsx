import { useNavigate } from 'react-router-dom'
import { MapPin } from 'lucide-react'
import BookCover from './BookCover.jsx'
import Badge from '../ui/Badge.jsx'
import { useLibrary } from '../../store/LibraryContext.jsx'
import { availableCopiesOf, branchName } from '../../utils/selectors.js'

/**
 * Catalog card: cover, title, author, category, availability and a
 * "View Details" button. Availability is computed live from the store.
 * `layout="row"` renders the compact shelf (list) variant instead.
 */
export default function BookCard({ book, role = 'student', layout = 'grid' }) {
  const { state } = useLibrary()
  const navigate = useNavigate()
  const available = availableCopiesOf(state, book)
  const base = role === 'admin' ? '/admin/books' : '/student/books'

  if (layout === 'row') {
    return (
      <article className="shelf-row">
        <div className="shelf-cover">
          <BookCover book={book} size="sm" />
        </div>
        <div className="shelf-main">
          <h3 className="shelf-title">{book.title}</h3>
          <div className="shelf-author">{book.author}</div>
          <div className="shelf-meta">
            <Badge tone="gray">{book.category}</Badge>
            <span className="text-xs text-muted row" style={{ gap: 4 }}>
              <MapPin size={12} /> {branchName(state, book.branchId)}
            </span>
            <span className="text-xs text-muted">ISBN {book.isbn}</span>
          </div>
        </div>
        <div className="shelf-side">
          <Badge status={available > 0 ? 'available' : 'expired'}>
            {available > 0 ? `${available} of ${book.totalCopies} available` : 'All issued'}
          </Badge>
          <button
            className="btn btn-secondary btn-sm"
            onClick={() => navigate(`${base}/${book.id}`)}
          >
            View Details
          </button>
        </div>
      </article>
    )
  }

  return (
    <article className="book-card">
      <div className="book-cover-wrap">
        <BookCover book={book} />
      </div>
      <div className="book-card-body">
        <Badge tone="gray" className="category-chip">
          {book.category}
        </Badge>
        <h3 className="book-card-title mt-2">{book.title}</h3>
        <div className="book-card-author">{book.author}</div>

        <div className="text-xs text-muted mt-2 row" style={{ gap: 5 }}>
          <MapPin size={12} />
          {branchName(state, book.branchId)}
        </div>

        <div className="book-card-footer">
          <Badge status={available > 0 ? 'available' : 'expired'}>
            {available > 0 ? `${available} of ${book.totalCopies} available` : 'All copies issued'}
          </Badge>
        </div>
        <button
          className="btn btn-secondary btn-sm full-width mt-3"
          onClick={() => navigate(`${base}/${book.id}`)}
        >
          View Details
        </button>
      </div>
    </article>
  )
}
