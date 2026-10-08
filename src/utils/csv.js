/**
 * Dependency-free CSV helpers: parsing, generation and file download.
 * Used by the bulk import page (reading spreadsheets) and the reports
 * page (exporting report data).
 */

/** Parses CSV text into an array of rows (arrays of strings).
 *  Handles quoted fields, escaped quotes (""), commas/newlines inside
 *  quotes, CRLF line endings and a leading UTF-8 BOM. Blank rows are
 *  dropped. */
export function parseCsv(text) {
  const src = String(text ?? '').replace(/^\uFEFF/, '')
  const rows = []
  let row = []
  let field = ''
  let inQuotes = false

  for (let i = 0; i < src.length; i++) {
    const ch = src[i]
    if (inQuotes) {
      if (ch === '"') {
        if (src[i + 1] === '"') {
          field += '"'
          i++
        } else {
          inQuotes = false
        }
      } else {
        field += ch
      }
    } else if (ch === '"') {
      inQuotes = true
    } else if (ch === ',') {
      row.push(field)
      field = ''
    } else if (ch === '\n' || ch === '\r') {
      if (ch === '\r' && src[i + 1] === '\n') i++
      row.push(field)
      field = ''
      rows.push(row)
      row = []
    } else {
      field += ch
    }
  }
  if (field !== '' || row.length > 0) {
    row.push(field)
    rows.push(row)
  }
  return rows.filter((r) => r.some((cell) => cell.trim() !== ''))
}

/**
 * Escapes a single CSV field.
 * Cells that spreadsheet apps would treat as formulas (=, +, -, @, tab, CR
 * at the start) are prefixed with an apostrophe so exported reports can
 * never trigger CSV/formula injection. Cells that parse as plain numbers
 * (e.g. -5) are left untouched so negative values stay numeric.
 */
const FORMULA_START = /^[=+@\t\r-]/

function escapeCell(value) {
  let s = String(value ?? '')
  const isNumber = s.trim() !== '' && Number.isFinite(Number(s))
  if (!isNumber && FORMULA_START.test(s)) s = `'${s}`
  return /[",\n\r]/.test(s) ? `"${s.replace(/"/g, '""')}"` : s
}

/** Builds CSV text from a header row and data rows (RFC 4180 style). */
export function toCsv(headers, rows = []) {
  const lines = [headers.map(escapeCell).join(',')]
  for (const row of rows) lines.push(row.map(escapeCell).join(','))
  return lines.join('\r\n')
}

/** Triggers a browser download of `content` as a file. */
export function downloadTextFile(filename, content, mime = 'text/csv;charset=utf-8') {
  const blob = new Blob([`\uFEFF${content}`], { type: mime })
  const url = URL.createObjectURL(blob)
  const link = document.createElement('a')
  link.href = url
  link.download = filename
  document.body.appendChild(link)
  link.click()
  link.remove()
  setTimeout(() => URL.revokeObjectURL(url), 1000)
}
