/** Status pill. Pass a known status (or an explicit colour + children). */
const TONES = {
  available: 'badge-success',
  active: 'badge-success',
  success: 'badge-success',
  paid: 'badge-success',
  returned: 'badge-success',
  fulfilled: 'badge-success',
  issued: 'badge-info',
  info: 'badge-info',
  ready: 'badge-primary',
  waiting: 'badge-warning',
  pending: 'badge-warning',
  due: 'badge-warning',
  'due-soon': 'badge-warning',
  overdue: 'badge-danger',
  expired: 'badge-danger',
  failed: 'badge-danger',
  suspended: 'badge-danger',
  danger: 'badge-danger',
  unread: 'badge-primary',
  read: 'badge-gray',
  skipped: 'badge-gray',
  inactive: 'badge-gray',
}

const LABELS = {
  'due-soon': 'Due soon',
}

export default function Badge({ status, tone, children, className = '' }) {
  const key = tone || status || 'gray'
  const cls = TONES[key] || 'badge-gray'
  const label = children || LABELS[status] || (status ? String(status) : '')
  return <span className={`badge ${cls} ${className}`.trim()}>{label}</span>
}
