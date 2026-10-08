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
})
