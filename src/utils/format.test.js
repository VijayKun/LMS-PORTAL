import { describe, it, expect } from 'vitest'
import { formatDate, addDays, daysBetween, getInitials, formatCurrency, relativeDay } from './format.js'

describe('formatDate', () => {
  it('formats ISO dates for display', () => {
    expect(formatDate('2026-10-12')).toBe('12 Oct 2026')
  })

  it('returns an em dash for empty or invalid input', () => {
    expect(formatDate('')).toBe('—')
    expect(formatDate(null)).toBe('—')
    expect(formatDate('not-a-date')).toBe('—')
  })
})

describe('addDays / daysBetween', () => {
  it('adds days across month boundaries', () => {
    expect(addDays('2026-01-31', 1)).toBe('2026-02-01')
  })

  it('subtracts days', () => {
    expect(addDays('2026-03-01', -1)).toBe('2026-02-28')
  })

  it('measures the distance between two dates', () => {
    expect(daysBetween('2026-01-01', '2026-01-11')).toBe(10)
    expect(daysBetween('2026-01-11', '2026-01-01')).toBe(-10)
    expect(daysBetween('2026-01-01', '2026-01-01')).toBe(0)
  })
})

describe('getInitials', () => {
  it('takes the first letters of the first two names', () => {
    expect(getInitials('Shivangi Verma')).toBe('SV')
    expect(getInitials('Meenakshi Raman')).toBe('MR')
  })

  it('handles single words and empty input', () => {
    expect(getInitials('Cher')).toBe('C')
    expect(getInitials('')).toBe('')
  })
})

describe('formatCurrency', () => {
  it('prefixes the rupee symbol', () => {
    expect(formatCurrency(30)).toBe('₹30')
    expect(formatCurrency(0)).toBe('₹0')
  })
})

describe('relativeDay', () => {
  it('labels today and yesterday', () => {
    const now = new Date()
    const iso = (d) => new Date(d.getTime() - d.getTimezoneOffset() * 60000).toISOString().slice(0, 10)
    expect(relativeDay(iso(now))).toBe('Today')
    expect(relativeDay(iso(new Date(now.getTime() - 86400000)))).toBe('Yesterday')
  })
})
