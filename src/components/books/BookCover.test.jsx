import { describe, it, expect, vi, afterEach } from 'vitest'
import { render, screen, waitFor } from '@testing-library/react'

const BOOK = {
  id: 'bk1',
  title: 'Clean Code',
  author: 'Robert C. Martin',
  isbn: '9780132350884',
  cover: '#2F4858',
}

async function loadCover() {
  vi.resetModules()
  return (await import('./BookCover.jsx')).default
}

function mockLookup(payload) {
  const fetchMock = vi.fn().mockResolvedValue({
    ok: true,
    json: async () => payload,
  })
  vi.stubGlobal('fetch', fetchMock)
  return fetchMock
}

afterEach(() => {
  vi.unstubAllGlobals()
  sessionStorage.clear()
})

describe('BookCover', () => {
  it('renders a cover already stored as an image URL without calling the backend', async () => {
    const fetchMock = vi.fn()
    vi.stubGlobal('fetch', fetchMock)
    const BookCover = await loadCover()

    render(<BookCover book={{ ...BOOK, cover: 'http://example.com/clean-code.jpg' }} />)

    expect(screen.getByRole('img').getAttribute('src')).toBe(
      'https://example.com/clean-code.jpg',
    )
    expect(fetchMock).not.toHaveBeenCalled()
  })

  it('looks a cover up through the backend and caches the result', async () => {
    const fetchMock = mockLookup({
      found: true,
      cover: 'http://books.google.com/books/content?id=abc',
    })
    const BookCover = await loadCover()

    const first = render(<BookCover book={BOOK} />)
    await waitFor(() =>
      expect(screen.getByRole('img').getAttribute('src')).toBe(
        'https://books.google.com/books/content?id=abc',
      ),
    )
    expect(fetchMock).toHaveBeenCalledTimes(1)
    expect(fetchMock.mock.calls[0][0]).toContain('/api/books/lookup/9780132350884')
    first.unmount()

    render(<BookCover book={BOOK} />)
    await waitFor(() =>
      expect(screen.getByRole('img').getAttribute('src')).toBe(
        'https://books.google.com/books/content?id=abc',
      ),
    )
    expect(fetchMock).toHaveBeenCalledTimes(1)
  })

  it('reuses a persisted cover after the module reloads', async () => {
    const fetchMock = mockLookup({
      found: true,
      cover: 'https://books.google.com/books/content?id=abc',
    })
    let BookCover = await loadCover()
    const first = render(<BookCover book={BOOK} />)
    await waitFor(() => expect(screen.getByRole('img')).toBeTruthy())
    expect(fetchMock).toHaveBeenCalledTimes(1)
    first.unmount()

    BookCover = await loadCover()
    render(<BookCover book={BOOK} />)
    await waitFor(() => expect(screen.getByRole('img')).toBeTruthy())
    expect(fetchMock).toHaveBeenCalledTimes(1)
  })

  it('falls back to the coloured tile when no cover is found', async () => {
    const fetchMock = mockLookup({ found: false })
    const BookCover = await loadCover()

    render(<BookCover book={BOOK} />)

    await waitFor(() => expect(fetchMock).toHaveBeenCalledTimes(1))
    expect(screen.queryByRole('img')).toBeNull()
    expect(screen.getByText('Clean Code')).toBeTruthy()
  })
})
