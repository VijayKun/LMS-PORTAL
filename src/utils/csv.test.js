import { describe, it, expect } from 'vitest'
import { parseCsv, toCsv } from './csv.js'

describe('parseCsv', () => {
  it('parses simple rows', () => {
    expect(parseCsv('a,b\n1,2')).toEqual([
      ['a', 'b'],
      ['1', '2'],
    ])
  })

  it('keeps commas inside quoted fields', () => {
    expect(parseCsv('"Martin, Robert",9780132350884')).toEqual([
      ['Martin, Robert', '9780132350884'],
    ])
  })

  it('unescapes doubled quotes', () => {
    expect(parseCsv('"He said ""hi""",x')).toEqual([['He said "hi"', 'x']])
  })

  it('keeps newlines inside quoted fields', () => {
    expect(parseCsv('"line one\nline two",y')).toEqual([['line one\nline two', 'y']])
  })

  it('handles CRLF line endings and a trailing newline', () => {
    expect(parseCsv('a,b\r\n1,2\r\n')).toEqual([
      ['a', 'b'],
      ['1', '2'],
    ])
  })

  it('strips a UTF-8 BOM', () => {
    expect(parseCsv('\uFEFFtitle,isbn')).toEqual([['title', 'isbn']])
  })

  it('drops blank lines', () => {
    expect(parseCsv('a,b\n\n1,2\n   \n')).toEqual([
      ['a', 'b'],
      ['1', '2'],
    ])
  })

  it('returns an empty array for empty input', () => {
    expect(parseCsv('')).toEqual([])
  })
})

describe('toCsv', () => {
  it('joins plain values', () => {
    expect(toCsv(['a', 'b'], [['1', '2']])).toBe('a,b\r\n1,2')
  })

  it('quotes values containing commas, quotes and newlines', () => {
    const csv = toCsv(['name'], [['Clean, "Code"\nvol 1']])
    expect(csv).toBe('name\r\n"Clean, ""Code""\nvol 1"')
  })

  it('round-trips through parseCsv', () => {
    const headers = ['Book Name', 'Author']
    const rows = [['Clean Code', 'Robert C. Martin'], ['Dune, Part One', 'Frank "Herbert"']]
    expect(parseCsv(toCsv(headers, rows))).toEqual([headers, ...rows])
  })

  it('neutralises formula injection in exported cells', () => {
    expect(toCsv(['note'], [['=1+2']])).toBe('note\r\n\'=1+2')
    expect(toCsv(['note'], [['+cmd|calc']])).toBe('note\r\n\'+cmd|calc')
    expect(toCsv(['note'], [['@SUM(A1)']])).toBe('note\r\n\'@SUM(A1)')
    expect(toCsv(['note'], [['-2+3+cmd']])).toBe('note\r\n\'-2+3+cmd')
    expect(toCsv(['note'], [['\tcmd']])).toBe('note\r\n\'\tcmd')
  })

  it('keeps negative numbers and plain text untouched', () => {
    expect(toCsv(['value'], [[-5]])).toBe('value\r\n-5')
    expect(toCsv(['value'], [[-1.5]])).toBe('value\r\n-1.5')
    expect(toCsv(['value'], [['Clean Code']])).toBe('value\r\nClean Code')
  })
})
