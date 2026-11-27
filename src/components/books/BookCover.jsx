/**
 * Book cover tile.
 *
 * Resolves a cover in this order:
 *   1. `book.coverLarge` / `book.cover` when it already holds an http(s) URL
 *   2. the backend Google Books proxy (`GET /api/books/lookup/:isbn`)
 *   3. the colour stored in `book.cover`, rendered as a stylised tile
 *
 * Lookups are cached per ISBN - hits and misses alike - so a catalog page asks
 * the backend once per book instead of firing a keyless Google request for
 * every card (which hits the public rate limit and returns HTTP 429).
 */
import { useEffect, useState } from 'react'
import { API_BASE } from '../../services/backend.js'

const HTTP_URL = /^https?:\/\//i
const LOOKUP_TIMEOUT_MS = 8000
const STORAGE_PREFIX = 'lms.cover.'

const coverCache = new Map()

function normalizeCoverUrl(value) {
  const url = typeof value === 'string' ? value.trim() : ''
  return HTTP_URL.test(url) ? url.replace(/^http:/i, 'https:') : null
}

/* Survives reloads within the tab, so a refreshed catalog is free. */
function readCachedCover(key) {
  try {
    return normalizeCoverUrl(sessionStorage.getItem(`${STORAGE_PREFIX}${key}`))
  } catch {
    return null
  }
}

function writeCachedCover(key, url) {
  try {
    sessionStorage.setItem(`${STORAGE_PREFIX}${key}`, url)
  } catch {
    /* private mode / storage disabled - the in-memory cache still works */
  }
}

function storedCoverUrl(book) {
  return normalizeCoverUrl(book?.coverLarge) || normalizeCoverUrl(book?.cover)
}

function lookupCover(isbn) {
  const key = String(isbn || '').replace(/[-\s]/g, '')
  if (!key) return Promise.resolve(null)

  if (!coverCache.has(key)) {
    const persisted = readCachedCover(key)
    if (persisted) {
      coverCache.set(key, Promise.resolve(persisted))
    } else {
      const controller = new AbortController()
      const timer = setTimeout(() => controller.abort(), LOOKUP_TIMEOUT_MS)
      const pending = fetch(`${API_BASE}/api/books/lookup/${encodeURIComponent(key)}`, {
        headers: { Accept: 'application/json' },
        signal: controller.signal,
      })
        .then((res) => (res.ok ? res.json() : null))
        .then((data) => {
          const url = data?.found ? normalizeCoverUrl(data.cover) : null
          if (url) writeCachedCover(key, url)
          return url
        })
        .catch(() => null)
        .finally(() => clearTimeout(timer))
      coverCache.set(key, pending)
    }
  }

  return coverCache.get(key)
}

export default function BookCover({ book, size = 'md', className = '' }) {
  const classes = [
    'book-cover',
    size === 'sm' ? 'cover-sm' : '',
    size === 'lg' ? 'cover-lg' : '',
    className,
  ]
    .filter(Boolean)
    .join(' ')

  const stored = storedCoverUrl(book)
  const isbn = book?.isbn || ''
  const coverField = typeof book?.cover === 'string' ? book.cover.trim() : ''
  const fallbackColor = coverField && !normalizeCoverUrl(coverField) ? coverField : '#37474F'

  const [lookup, setLookup] = useState({ isbn: '', url: null })
  const [failedUrl, setFailedUrl] = useState(null)

  useEffect(() => {
    if (stored || !isbn) return undefined

    let active = true
    lookupCover(isbn).then((url) => {
      if (active && url) setLookup({ isbn, url })
    })

    return () => {
      active = false
    }
  }, [isbn, stored])

  const lookedUpUrl = lookup.isbn === isbn ? lookup.url : null
  const resolvedUrl = stored || lookedUpUrl
  const coverUrl = resolvedUrl && resolvedUrl !== failedUrl ? resolvedUrl : null

  if (coverUrl) {
    return (
      <div className={classes}>
        <img
          className="cover-img"
          src={coverUrl}
          alt={`${book?.title || 'Book'} cover`}
          loading="lazy"
          decoding="async"
          onError={() => setFailedUrl(coverUrl)}
        />
      </div>
    )
  }

  return (
    <div className={classes} style={{ background: fallbackColor }} aria-hidden="true">
      <span className="cover-sheen" />
      <div className="cover-title">{book?.title}</div>
      <div className="cover-author">{book?.author}</div>
    </div>
  )
}
