/**
 * Book cover tile.
 * Uses the colour stored in `book.cover` until the backend provides real
 * cover image URLs - if `cover` is an http(s) URL an <img> is rendered instead.
 * A layered sheen + spine highlight sit above the flat colour to give the
 * tile a physical, book-like feel in both light and dark themes.
 */
export default function BookCover({ book, size = 'md', className = '' }) {
  const classes = [
    'book-cover',
    size === 'sm' ? 'cover-sm' : '',
    size === 'lg' ? 'cover-lg' : '',
    className,
  ]
    .filter(Boolean)
    .join(' ')

  if (typeof book.cover === 'string' && book.cover.startsWith('http')) {
    return (
      <div className={classes}>
        <img className="cover-img" src={book.cover} alt={`${book.title} cover`} />
      </div>
    )
  }

  return (
    <div className={classes} style={{ background: book.cover || '#37474F' }} aria-hidden="true">
      <span className="cover-sheen" />
      <div className="cover-title">{book.title}</div>
      <div className="cover-author">{book.author}</div>
    </div>
  )
}
