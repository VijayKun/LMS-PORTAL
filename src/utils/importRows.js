/**
 * Pure logic for the bulk import preview: maps spreadsheet rows onto
 * import candidates and validates them (ISBN checksum, duplicates,
 * required fields, branch/subject matching). No React, no side effects - kept
 * separate so it can be unit tested (see importRows.test.js).
 */

const normalize = (value) => String(value ?? '').toLowerCase().replace(/[^a-z0-9]/g, '')

/** Flexible header aliases so real-world spreadsheets map cleanly. */
const HEADER_ALIASES = {
  title: ['bookname', 'title', 'book', 'name', 'booktitle'],
  isbn: ['isbn', 'isbn13', 'isbn10'],
  copies: ['copies', 'qty', 'quantity', 'totalcopies', 'numberofcopies', 'count'],
  branch: ['branch', 'branchname', 'location', 'librarybranch', 'subject', 'department', 'discipline'],
  author: ['author', 'authors', 'writer'],
  category: ['category', 'genre', 'subject', 'department'],
  publisher: ['publisher', 'press'],
}

/** Length caps so a hostile spreadsheet can't inject absurd cell values. */
const MAX_FIELD_LENGTHS = { title: 300, author: 200, category: 100, publisher: 200 }
const MAX_COPIES = 9999

/** Maps a header row to column indexes: { title: 0, isbn: 1, ... } */
export function mapHeaders(headerRow) {
  if (!Array.isArray(headerRow)) {
    throw new Error('Header row must be an array of column names')
  }
  const map = {}
  headerRow.forEach((cell, index) => {
    const key = normalize(cell)
    for (const [field, aliases] of Object.entries(HEADER_ALIASES)) {
      if (map[field] === undefined && aliases.includes(key)) {
        map[field] = index
        break
      }
    }
  })
  return map
}

/** Validates ISBN-10 / ISBN-13 including checksum. Hyphens and spaces allowed. */
export function isValidIsbn(raw) {
  const isbn = String(raw ?? '').replace(/[-\s]/g, '')
  if (/^\d{9}[\dX]$/.test(isbn)) {
    let sum = 0
    for (let i = 0; i < 10; i++) sum += (isbn[i] === 'X' ? 10 : Number(isbn[i])) * (10 - i)
    return sum % 11 === 0
  }
  if (/^\d{13}$/.test(isbn)) {
    let sum = 0
    for (let i = 0; i < 13; i++) sum += Number(isbn[i]) * (i % 2 === 1 ? 3 : 1)
    return sum % 10 === 0
  }
  return false
}

/**
 * Turns raw table rows (first row = headers) into validated import candidates.
 *
 * @param {string[][]} table      rows from parseCsv() or read-excel-file
 * @param {object} options
 * @param {Array} options.existingBooks  catalog books (duplicate ISBN detection)
 * @param {Array} options.branches       academic branches/subjects (name/id matching)
 * @returns {Array<{key,title,isbn,author,category,publisher,copies,branchId,
 *                  branchLabel,initial,result,reason}>}
 * @throws {Error} when required columns are missing
 */
export function buildImportRows(table, { existingBooks = [], branches = [] } = {}) {
  if (!Array.isArray(table) || table.length < 2) {
    throw new Error('The file has no data rows below the header.')
  }
  const header = mapHeaders(table[0])
  if (header.title === undefined || header.isbn === undefined) {
    throw new Error('Missing required columns. Expected headers like: Book Name, ISBN, Copies, Branch.')
  }

  const existing = new Set(
    existingBooks.map((b) => String(b.isbn ?? '').replace(/[-\s]/g, '').toLowerCase()),
  )
  const seen = new Set()

  if (!Array.isArray(table[0])) {
    throw new Error('Header row must be an array of column names')
  }
  return table.slice(1).map((cells, index) => {
    if (!Array.isArray(cells)) {
      throw new Error(`Data row ${index + 1} is not properly formatted`)
    }
    const get = (field) =>
      header[field] === undefined ? '' : String(cells[header[field]] ?? '').trim()

    const title = get('title')
    const isbn = get('isbn')
    const isbnKey = isbn.replace(/[-\s]/g, '').toLowerCase()
    const author = get('author')
    const category = get('category')
    const publisher = get('publisher')
    const copies = Math.min(MAX_COPIES, Math.max(1, parseInt(get('copies'), 10) || 1))
    const branchLabel = get('branch')
    const branch =
      branches.find(
        (b) =>
          (branchLabel && normalize(b.name) === normalize(branchLabel)) ||
          (branchLabel && b.id === branchLabel),
      ) || branches[0]

    const oversized = Object.entries(MAX_FIELD_LENGTHS).find(
      ([field, max]) => String({ title, author, category, publisher }[field]).length > max,
    )

    let status = 'ready'
    if (!title) status = 'missing-title'
    else if (oversized) status = 'too-long'
    else if (!isValidIsbn(isbn)) status = 'bad-isbn'
    else if (seen.has(isbnKey) || existing.has(isbnKey)) status = 'duplicate'
    if (isbnKey) seen.add(isbnKey)

    const reason =
      status === 'missing-title'
        ? 'Book name is empty'
        : status === 'too-long'
          ? `Field "${oversized[0]}" exceeds ${oversized[1]} characters`
          : status === 'bad-isbn'
            ? 'Invalid ISBN format or checksum'
            : status === 'duplicate'
              ? 'ISBN already in the catalog or repeated in the file'
              : null

    return {
      key: index,
      title: title || '(empty title)',
      isbn: isbn || '—',
      author: author || 'Unknown',
      category: category || 'General',
      publisher: publisher || '—',
      copies,
      branchId: branch?.id ?? null,
      branchLabel: branch?.name ?? 'Unmatched',
      initial: status,
      result: status === 'ready' ? 'imported' : status === 'duplicate' ? 'skipped' : 'failed',
      reason,
    }
  })
}
