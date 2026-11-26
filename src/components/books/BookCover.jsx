/**
 * Book cover tile.
 * Uses the colour stored in `book.cover` until the backend provides real
 * cover image URLs - if `cover` is an http(s) URL an <img> is rendered instead.
 * A layered sheen + spine highlight sit above the flat colour to give the
 * tile a physical, book-like feel in both light and dark themes.
 */
import { useEffect, useState } from 'react'

export default function BookCover({ book, size = 'md', className = '' }) {
  const classes = [
    'book-cover',
    size === 'sm' ? 'cover-sm' : '',
    size === 'lg' ? 'cover-lg' : '',
    className,
  ]
    .filter(Boolean)
    .join(' ')

  const [coverUrl, setCoverUrl] = useState(null)

  useEffect(() => {
    if (!book || !book.isbn) return

    const fetchCover = async () => {
      const isbn = book.isbn.replace(/[-\s]/g, '')
      try {
        const res = await fetch(
          `https://www.googleapis.com/books/v1/volumes?q=isbn:${encodeURIComponent(isbn)}`,
        )
        if (res.ok) {
          const data = await res.json()
          const volume = data?.items?.[0]?.volumeInfo
          const links = volume?.imageLinks || {}
          const thumb = links.thumbnail || links.smallThumbnail || ''
          if (thumb) {
            setCoverUrl(thumb.replace(/^http:/, 'https:'))
          }
        }
      } catch {
        // Fall back to default
      }
    }

    fetchCover()
  }, [book?.isbn])

  if (coverUrl) {
    return (
      <div className={classes}>
        <img
          className="cover-img"
          src={coverUrl}
          alt={`${book.title} cover`}
          onError={(e) => {
            e.target.onerror = null
            e.target.src = ''
          }}
        />
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
