import { describe, it, expect } from 'vitest'
import { mapHeaders, isValidIsbn, buildImportRows } from './importRows.js'
import { toBookImportPayload } from '../services/mappers.js'

const branches = [
  { id: 'cs', name: 'Computer Science & Engineering' },
  { id: 'it', name: 'Information Technology' },
]

describe('mapHeaders', () => {
  it('matches common header aliases case-insensitively', () => {
    const map = mapHeaders(['Book Title', 'ISBN', 'Qty', 'Location', 'Author'])
    expect(map).toMatchObject({ title: 0, isbn: 1, copies: 2, branch: 3, author: 4 })
  })

  it('ignores unknown columns', () => {
    const map = mapHeaders(['Notes', 'Title'])
    expect(map.title).toBe(1)
    expect(map.copies).toBeUndefined()
  })
})

describe('isValidIsbn', () => {
  it('accepts a valid ISBN-13 (with hyphens)', () => {
    expect(isValidIsbn('978-0-13-235088-4')).toBe(true)
  })

  it('accepts a valid ISBN-10', () => {
    expect(isValidIsbn('0136350887')).toBe(true)
  })

  it('rejects a broken checksum', () => {
    expect(isValidIsbn('9780132350885')).toBe(false)
  })

  it('rejects malformed input', () => {
    expect(isValidIsbn('ISBN-INVALID')).toBe(false)
    expect(isValidIsbn('')).toBe(false)
    expect(isValidIsbn('978013')).toBe(false)
  })
})

describe('buildImportRows', () => {
  const table = [
    ['Book Name', 'ISBN', 'Copies', 'Branch'],
    ['Clean Code', '9780132350884', '3', 'Computer Science & Engineering'],
    ['Refactoring', '9780134757599', '', 'it'],
  ]

  it('maps valid rows to ready import candidates', () => {
    const rows = buildImportRows(table, { existingBooks: [], branches })
    expect(rows).toHaveLength(2)
    expect(rows[0]).toMatchObject({
      title: 'Clean Code',
      copies: 3,
      branchId: 'cs',
      initial: 'ready',
      result: 'imported',
    })
    expect(rows[1]).toMatchObject({ branchId: 'it', copies: 1 })
  })

  it('marks bad ISBNs as failed', () => {
    const rows = buildImportRows(
      [['Book Name', 'ISBN'], ['Broken', '123']],
      { existingBooks: [], branches },
    )
    expect(rows[0].initial).toBe('bad-isbn')
    expect(rows[0].result).toBe('failed')
    expect(rows[0].reason).toMatch(/ISBN/)
  })

  it('marks empty titles as failed', () => {
    const rows = buildImportRows(
      [['Book Name', 'ISBN'], ['', '9780132350884']],
      { existingBooks: [], branches },
    )
    expect(rows[0].initial).toBe('missing-title')
  })

  it('skips ISBNs already in the catalog', () => {
    const rows = buildImportRows(table, {
      existingBooks: [{ isbn: '9780132350884' }],
      branches,
    })
    expect(rows[0].initial).toBe('duplicate')
    expect(rows[0].result).toBe('skipped')
    expect(rows[1].initial).toBe('ready')
  })

  it('skips repeats within the same file', () => {
    const rows = buildImportRows(
      [
        ['Book Name', 'ISBN'],
        ['A', '9780132350884'],
        ['B', '978-0-13-235088-4'],
      ],
      { existingBooks: [], branches },
    )
    expect(rows[0].initial).toBe('ready')
    expect(rows[1].initial).toBe('duplicate')
  })

  it('throws when required columns are missing', () => {
    expect(() => buildImportRows([['Notes'], ['hello']], { branches })).toThrow(/Missing required columns/)
  })

  it('throws when the file has no data rows', () => {
    expect(() => buildImportRows([['Book Name', 'ISBN']], { branches })).toThrow(/no data rows/)
  })

  it('marks absurdly long field values as failed', () => {
    const rows = buildImportRows(
      [['Book Name', 'ISBN'], ['x'.repeat(301), '9780132350884']],
      { existingBooks: [], branches },
    )
    expect(rows[0].initial).toBe('too-long')
    expect(rows[0].result).toBe('failed')
    expect(rows[0].reason).toMatch(/exceeds 300 characters/)
  })

  it('clamps the copies count to a sane maximum', () => {
    const rows = buildImportRows(
      [['Book Name', 'ISBN', 'Copies'], ['Clean Code', '9780132350884', '99999999']],
      { branches },
    )
    expect(rows[0].copies).toBe(9999)
    expect(rows[0].initial).toBe('ready')
  })
})

describe('toBookImportPayload', () => {
  it('sends the field names and string counts the backend reads', () => {
    const payload = toBookImportPayload({
      title: 'Clean Code',
      isbn: '9780132350884',
      author: 'Robert C. Martin',
      category: 'Computers',
      publisher: 'Pearson Education',
      copies: 3,
      branchId: 'cs',
      publicationDate: '2009-01-01',
      description: 'A book about code',
      cover: 'https://books.google.com/cover.jpg',
    })
    expect(payload).toMatchObject({
      title: 'Clean Code',
      totalCopies: '3',
      availableCopies: '3',
      publicationDate: '2009-01-01',
      year: '2009',
      branchId: 'cs',
    })
    expect(payload).not.toHaveProperty('copies')
    expect(payload).not.toHaveProperty('initial')
  })

  it('normalises partial Google Books dates and defaults bad counts', () => {
    expect(toBookImportPayload({ publicationDate: '2009' }).publicationDate).toBe('2009-01-01')
    expect(toBookImportPayload({ publicationDate: '2009-03' }).publicationDate).toBe('2009-03-01')
    const payload = toBookImportPayload({ copies: 'not-a-number' })
    expect(payload.totalCopies).toBe('1')
    expect(payload.availableCopies).toBe('1')
    expect(payload.publicationDate).toBe('')
  })
})

