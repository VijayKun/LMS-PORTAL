/**
 * Small formatting helpers shared by every page.
 * Pure functions only - no framework, no side effects.
 */

const MONTHS = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec']

/** '2026-10-12' -> '12 Oct 2026' */
export function formatDate(iso) {
  if (!iso) return '—'
  const d = parseDate(iso)
  if (Number.isNaN(d.getTime())) return '—'
  return `${d.getUTCDate()} ${MONTHS[d.getUTCMonth()]} ${d.getUTCFullYear()}`
}

/** '2026-10-12' -> '12 Oct' (short form for dense tables) */
export function formatDateShort(iso) {
  if (!iso) return '—'
  const d = parseDate(iso)
  if (Number.isNaN(d.getTime())) return '—'
  return `${d.getUTCDate()} ${MONTHS[d.getUTCMonth()]}`
}

/** Parses 'YYYY-MM-DD' (and full timestamps) into a Date object. */
export function parseDate(iso) {
  if (typeof iso === 'string' && iso.length === 10) {
    return new Date(`${iso}T00:00:00Z`)
  }
  return new Date(iso)
}

/** Today's date as 'YYYY-MM-DD'. */
export function todayISO() {
  const d = new Date()
  return new Date(d.getTime() - d.getTimezoneOffset() * 60000).toISOString().slice(0, 10)
}

/** Days between two 'YYYY-MM-DD' strings (positive when `to` is later). */
export function daysBetween(fromISO, toISO) {
  const ms = parseDate(toISO).getTime() - parseDate(fromISO).getTime()
  return Math.round(ms / 86400000)
}

/** Days left until the due date. Negative means overdue. */
export function daysRemaining(dueISO) {
  return daysBetween(todayISO(), dueISO)
}

/** Adds `days` to a 'YYYY-MM-DD' string. */
export function addDays(iso, days) {
  const d = parseDate(iso)
  d.setUTCDate(d.getUTCDate() + days)
  return d.toISOString().slice(0, 10)
}

/** ₹ formatting used across fines and reports. */
export function formatCurrency(amount) {
  return `₹${Number(amount || 0).toLocaleString('en-IN')}`
}

/** Student year a book is meant for: 1 -> '1st year', 0/empty -> 'All years'. */
export function bookYearLabel(year) {
  const y = Number(year) || 0
  if (y <= 0) return 'All years'
  if (y === 1) return '1st year'
  if (y === 2) return '2nd year'
  if (y === 3) return '3rd year'
  return `${y}th year`
}

/** 'Shivangi Verma' -> 'SV' */
export function getInitials(name = '') {
  return name
    .split(' ')
    .filter(Boolean)
    .slice(0, 2)
    .map((part) => part[0].toUpperCase())
    .join('')
}

/** Relative label for notification timestamps: 'Today', 'Yesterday', '2 days ago'. */
export function relativeDay(iso) {
  const diff = daysBetween(iso.slice(0, 10), todayISO())
  if (diff <= 0) return 'Today'
  if (diff === 1) return 'Yesterday'
  if (diff < 7) return `${diff} days ago`
  return formatDate(iso.slice(0, 10))
}

export function formatNumber(value) {
  return Number(value || 0).toLocaleString('en-IN')
}

/** Time-of-day greeting: 'Good morning' / 'Good afternoon' / 'Good evening'. */
export function timeGreeting() {
  const hour = new Date().getHours()
  if (hour < 12) return 'Good morning'
  if (hour < 17) return 'Good afternoon'
  return 'Good evening'
}

/** 'Wednesday, 7 Oct 2026' - used in headers and dashboards. */
export function formatLongDate(iso = todayISO()) {
  const d = parseDate(iso)
  if (Number.isNaN(d.getTime())) return '—'
  const WEEKDAYS = ['Sunday', 'Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday']
  return `${WEEKDAYS[d.getUTCDay()]}, ${d.getUTCDate()} ${MONTHS[d.getUTCMonth()]} ${d.getUTCFullYear()}`
}
