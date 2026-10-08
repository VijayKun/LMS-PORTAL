import { useState } from 'react'
import { useParams, useNavigate, Link } from 'react-router-dom'
import {
  ArrowLeft,
  Bookmark,
  BookOpen,
  CheckCircle2,
  MapPin,
  Star,
} from 'lucide-react'
import { useLibrary } from '../../store/LibraryContext.jsx'
import { useAuth } from '../../store/AuthContext.jsx'
import { usePageTitle } from '../../store/PageTitleContext.jsx'
import { useToast } from '../../store/ToastContext.jsx'
import BookCover from '../../components/books/BookCover.jsx'
import BookCard from '../../components/books/BookCard.jsx'
import Badge from '../../components/ui/Badge.jsx'
import EmptyState from '../../components/ui/EmptyState.jsx'
import ConfirmDialog from '../../components/ui/ConfirmDialog.jsx'
import CopyText from '../../components/ui/CopyText.jsx'
import {
  availableCopiesOf,
  branchName,
  activeLoansOf,
  reservationsOf,
} from '../../utils/selectors.js'
import { formatDate, addDays, todayISO, bookYearLabel, getInitials } from '../../utils/format.js'
import { LOAN_PERIOD_DAYS, FINE_PER_DAY, MAX_BOOKS_PER_STUDENT } from '../../data/constants.js'
import { borrowBookRequest, reserveBookRequest } from '../../services/api.js'

const RATINGS_KEY = 'lms.ratings'

/** Deterministic mock score so every book has stable "community" ratings. */
function mockRating(book) {
  const seed = [...book.id].reduce((sum, ch) => sum + ch.charCodeAt(0), 0)
  const score = 3 + (seed % 21) / 10 // 3.0 - 5.0
  const count = 12 + (seed % 87)
  return { score: Math.round(score * 10) / 10, count }
}

function loadRatings() {
  try {
    return JSON.parse(localStorage.getItem(RATINGS_KEY)) || {}
  } catch {
    return {}
  }
}

function Stars({ value, size = 16 }) {
  return (
    <span className="stars" aria-label={`${value} out of 5 stars`}>
      {[1, 2, 3, 4, 5].map((i) => (
        <Star
          key={i}
          size={size}
          className={i <= Math.round(value) ? 'star lit' : 'star'}
          strokeWidth={2}
        />
      ))}
    </span>
  )
}

export default function BookDetails() {
  const { id } = useParams()
  const navigate = useNavigate()
  const { state, borrowBook, reserveBook, track } = useLibrary()
  const { user } = useAuth()
  const { toast } = useToast()
  usePageTitle('Book Details')

  const [confirming, setConfirming] = useState(null) // 'borrow' | 'reserve'
  const [ratings, setRatings] = useState(loadRatings)
  const book = state.books.find((b) => b.id === id)

  if (!book) {
    return (
      <div className="card">
        <EmptyState
          icon={BookOpen}
          title="Book not found"
          message="This book may have been removed from the catalog."
          action={
            <Link className="btn btn-primary" to="/student/books">
              Back to catalog
            </Link>
          }
        />
      </div>
    )
  }

  const available = availableCopiesOf(state, book)
  const myLoans = activeLoansOf(state, user.id)
  const hasThisBook = myLoans.some((t) => t.bookId === book.id)
  const hasReserved = reservationsOf(state, user.id).some(
    (r) => r.bookId === book.id && (r.status === 'waiting' || r.status === 'ready'),
  )
  const atLimit = myLoans.length >= MAX_BOOKS_PER_STUDENT
  const dueDateIfBorrowed = addDays(todayISO(), LOAN_PERIOD_DAYS)
  const publicationYear = book.publicationDate ? book.publicationDate.slice(0, 4) : ''

  function handleBorrow() {
    borrowBook(user.id, book.id) // optimistic: the UI updates immediately
    track(borrowBookRequest()) // ...while the "server" call is in flight
    setConfirming(null)
    toast(`"${book.title}" borrowed successfully. Due ${formatDate(dueDateIfBorrowed)}.`, 'success')
  }

  function handleReserve() {
    reserveBook(user.id, book.id)
    track(reserveBookRequest())
    setConfirming(null)
    toast('Book reserved successfully. We will notify you when it is ready.', 'success')
  }

  const stockedBranches = state.branches
    .filter((b) => b.id === book.branchId)
    .map((b) => ({
      ...b,
      stocked: true,
      available: available,
      total: book.totalCopies,
    }))

  // ---- ratings + readers ----
  const rating = mockRating(book)
  const myRating = ratings[book.id] || 0
  const borrowers = [
    ...new Set(
      state.transactions.filter((t) => t.bookId === book.id).map((t) => t.studentId),
    ),
  ]
    .map((sid) => state.students.find((s) => s.id === sid))
    .filter(Boolean)
    .slice(0, 6)
  const extraReaders = Math.max(
    0,
    new Set(state.transactions.filter((t) => t.bookId === book.id).map((t) => t.studentId)).size - 6,
  )

  // ---- related books (same category) ----
  const relatedBooks = state.books
    .filter((b) => b.category === book.category && b.id !== book.id)
    .slice(0, 4)

  function rate(stars) {
    const next = { ...ratings, [book.id]: stars }
    setRatings(next)
    try {
      localStorage.setItem(RATINGS_KEY, JSON.stringify(next))
    } catch {
      /* storage unavailable - the rating applies for this session */
    }
    toast(`You rated "${book.title}" ${stars} star${stars === 1 ? '' : 's'}.`, 'success')
  }

  return (
    <div className="detail-wrap">
      <Link to="/student/books" className="detail-back">
        <ArrowLeft size={15} /> Back to Browse Books
      </Link>

      <div className="page-header detail-pg">
        <h1>Book Details</h1>
      </div>

      {/* ---------------- Main hero ---------------- */}
      <section className="card card-pad detail-hero">
        <div className="detail-hero-cover">
          <BookCover book={book} size="lg" />
        </div>

        <div className="detail-hero-body">
          <h2 className="detail-title">{book.title}</h2>
          <div className="detail-author">{book.author}</div>

          <div className="detail-badges">
            <Badge tone="gray">{book.category}</Badge>
            <Badge tone="gray">
              <MapPin size={11} /> {branchName(state, book.branchId)}
            </Badge>
          </div>

          <div className="detail-avail">
            <span className={`detail-dot ${available > 0 ? 'available' : 'unavailable'}`} />
            <span>{available > 0 ? 'Available' : 'Currently unavailable'}</span>
            <span className="copies">
              {available} of {book.totalCopies} copies available
            </span>
          </div>

          {book.description && <p className="detail-hero-desc">{book.description}</p>}

          <div className="detail-action">
            {hasThisBook ? (
              <>
                <div className="badge badge-info" style={{ padding: '8px 14px', fontSize: 13 }}>
                  <CheckCircle2 size={14} /> You have this book issued
                </div>
                <button className="btn btn-secondary btn-lg" onClick={() => navigate('/student/my-books')}>
                  View in My Books
                </button>
                <div className="detail-loan">Due {formatDate(myLoans.find((t) => t.bookId === book.id)?.dueDate)}</div>
              </>
            ) : available > 0 ? (
              <>
                <button
                  className="btn btn-primary btn-lg"
                  disabled={atLimit}
                  onClick={() => setConfirming('borrow')}
                >
                  <BookOpen size={17} /> Borrow Book
                </button>
                {atLimit && (
                  <div className="field-error">
                    You have reached the limit of {MAX_BOOKS_PER_STUDENT} books. Return a book first.
                  </div>
                )}
              </>
            ) : (
              <>
                <button
                  className="btn btn-primary btn-lg"
                  disabled={hasReserved}
                  onClick={() => setConfirming('reserve')}
                >
                  <Bookmark size={17} />
                  {hasReserved ? 'Already Reserved' : 'Reserve Book'}
                </button>
              </>
            )}

            <div className="detail-loan">
              Loan period: {LOAN_PERIOD_DAYS} days · Fine: ₹{FINE_PER_DAY}/day after due date
              {available > 0 && !hasThisBook && ` · If borrowed today, return by ${formatDate(dueDateIfBorrowed)}`}
            </div>
          </div>
        </div>
      </section>

      {/* ---------------- About this book ---------------- */}
      {book.description && (
        <section className="card card-pad detail-section">
          <h3 className="detail-section-title">About this book</h3>
          <p className="detail-about">{book.description}</p>
        </section>
      )}

      {/* ---------------- Book information ---------------- */}
      <section className="card card-pad detail-section">
        <h3 className="detail-section-title">Book information</h3>
        <div className="detail-info">
          {book.category && (
            <div>
              <span className="detail-info-label">Category</span>
              <span className="detail-info-value">{book.category}</span>
            </div>
          )}
          {book.publisher && (
            <div>
              <span className="detail-info-label">Publisher</span>
              <span className="detail-info-value">{book.publisher}</span>
            </div>
          )}
          {book.publicationDate && (
            <div>
              <span className="detail-info-label">Publication date</span>
              <span className="detail-info-value">{formatDate(book.publicationDate)}</span>
            </div>
          )}
          {publicationYear && (
            <div>
              <span className="detail-info-label">Publication year</span>
              <span className="detail-info-value">{publicationYear}</span>
            </div>
          )}
          {book.isbn && (
            <div>
              <span className="detail-info-label">ISBN</span>
              <span className="detail-info-value">
                <CopyText value={book.isbn} label="ISBN" />
              </span>
            </div>
          )}
          {book.year !== undefined && (
            <div>
              <span className="detail-info-label">Recommended year</span>
              <span className="detail-info-value">{bookYearLabel(book.year)}</span>
            </div>
          )}
          <div>
            <span className="detail-info-label">Branch</span>
            <span className="detail-info-value">{branchName(state, book.branchId)}</span>
          </div>
          <div>
            <span className="detail-info-label">Total copies</span>
            <span className="detail-info-value">{book.totalCopies}</span>
          </div>
          <div>
            <span className="detail-info-label">Available copies</span>
            <span className="detail-info-value">{available}</span>
          </div>
        </div>
      </section>

      {/* ---------------- Availability ---------------- */}
      <section className="card card-pad detail-section">
        <h3 className="detail-section-title">Availability</h3>
        {stockedBranches.map((b) => (
          <div className="detail-avail-row" key={b.id}>
            <span className="detail-avail-name">
              <MapPin size={14} /> {b.name}
            </span>
            <span className="detail-avail-state">
              <span className={`detail-dot ${b.available > 0 ? 'available' : 'unavailable'}`} />
              {b.available > 0 ? `${b.available} of ${b.total} copies available` : 'All copies issued'}
            </span>
          </div>
        ))}

        <div className="detail-avail-foot">
          {hasThisBook ? (
            <span className="badge badge-info">
              <CheckCircle2 size={13} /> You have this book issued
            </span>
          ) : available > 0 ? (
            <button className="btn btn-secondary btn-sm" disabled={atLimit} onClick={() => setConfirming('borrow')}>
              <BookOpen size={14} /> Borrow Book
            </button>
          ) : (
            <button className="btn btn-secondary btn-sm" disabled={hasReserved} onClick={() => setConfirming('reserve')}>
              <Bookmark size={14} /> {hasReserved ? 'Already Reserved' : 'Reserve Book'}
            </button>
          )}
          {available === 0 && !hasThisBook && (
            <span className="text-xs text-muted">
              You can reserve this title and you will be notified when a copy is returned.
            </span>
          )}
        </div>
      </section>

      {/* ---------------- Ratings & readers ---------------- */}
      <section className="card card-pad detail-section">
        <div className="row-between mb-3">
          <h3 className="detail-section-title" style={{ marginBottom: 0 }}>
            Ratings &amp; readers
          </h3>
          <span className="text-xs text-muted">{rating.count} ratings</span>
        </div>

        <div className="rating-row">
          <span className="rating-score">{rating.score.toFixed(1)}</span>
          <Stars value={rating.score} size={17} />
          <span className="text-xs text-muted">
            {myRating ? `Your rating: ${myRating}/5` : 'Tap a star to rate'}
          </span>
        </div>
        <div className="rate-actions" role="radiogroup" aria-label={`Rate ${book.title}`}>
          {[1, 2, 3, 4, 5].map((i) => (
            <button
              key={i}
              type="button"
              className={`rate-star ${i <= myRating ? 'active' : ''}`}
              onClick={() => rate(i)}
              aria-label={`Rate ${i} star${i === 1 ? '' : 's'}`}
              aria-pressed={i <= myRating}
            >
              <Star size={20} fill={i <= myRating ? 'currentColor' : 'none'} />
            </button>
          ))}
        </div>

        <hr className="divider" />

        <div className="row" style={{ gap: 12, flexWrap: 'wrap' }}>
          {borrowers.length > 0 ? (
            <>
              <div className="avatar-stack">
                {borrowers.map((s) => (
                  <span className="avatar avatar-xs" key={s.id} title={s.name}>
                    {getInitials(s.name)}
                  </span>
                ))}
                {extraReaders > 0 && <span className="avatar avatar-xs more">+{extraReaders}</span>}
              </div>
              <span className="text-xs text-muted">
                {borrowers.length + extraReaders} student
                {borrowers.length + extraReaders === 1 ? '' : 's'} borrowed this title
              </span>
            </>
          ) : (
            <span className="text-xs text-muted">Be the first to borrow this title.</span>
          )}
        </div>
      </section>

      {/* ---------------- Related books ---------------- */}
      {relatedBooks.length > 0 && (
        <section className="detail-section">
          <h3 className="section-title">Related books</h3>
          <p className="section-sub">More from the {book.category} category</p>
          <div className="book-grid mt-3">
            {relatedBooks.map((related) => (
              <BookCard key={related.id} book={related} role="student" />
            ))}
          </div>
        </section>
      )}

      <ConfirmDialog
        open={confirming === 'borrow'}
        title="Borrow this book?"
        danger={false}
        confirmLabel="Borrow Book"
        message={`"${book.title}" will be issued to you until ${formatDate(dueDateIfBorrowed)} (${LOAN_PERIOD_DAYS} days).`}
        onClose={() => setConfirming(null)}
        onConfirm={handleBorrow}
      />

      <ConfirmDialog
        open={confirming === 'reserve'}
        title="Reserve this book?"
        danger={false}
        confirmLabel="Reserve Book"
        message={`All copies are currently issued. We will add "${book.title}" to your reservation queue and notify you when it is available.`}
        onClose={() => setConfirming(null)}
        onConfirm={handleReserve}
      />
    </div>
  )
}