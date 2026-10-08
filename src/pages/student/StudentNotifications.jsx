import { useMemo, useState } from 'react'
import { Bell, CheckCheck } from 'lucide-react'
import { useLibrary } from '../../store/LibraryContext.jsx'
import { useAuth } from '../../store/AuthContext.jsx'
import { usePageTitle } from '../../store/PageTitleContext.jsx'
import { useToast } from '../../store/ToastContext.jsx'
import NotificationItem from '../../components/notifications/NotificationItem.jsx'
import EmptyState from '../../components/ui/EmptyState.jsx'
import { notificationsOf } from '../../utils/selectors.js'

const FILTERS = [
  { id: 'all', label: 'All' },
  { id: 'due', label: 'Due Reminders' },
  { id: 'overdue', label: 'Overdue' },
  { id: 'fine', label: 'Fines' },
  { id: 'reservation', label: 'Reservations' },
  { id: 'announcement', label: 'Announcements' },
]

export default function StudentNotifications() {
  usePageTitle('Notifications')
  const { state, markNotificationRead, markAllNotificationsRead } = useLibrary()
  const { user } = useAuth()
  const { toast } = useToast()

  const [filter, setFilter] = useState('all')
  const all = notificationsOf(state, user.id)
  const rows = useMemo(
    () => (filter === 'all' ? all : all.filter((n) => n.type === filter)),
    [all, filter],
  )
  const unread = all.filter((n) => !n.read).length

  function markAll() {
    markAllNotificationsRead(user.id)
    toast('All notifications marked as read.', 'success')
  }

  return (
    <>
      <div className="page-header">
        <div>
          <h1>Notifications</h1>
          <p className="page-sub">
            {unread > 0 ? `${unread} unread notification${unread === 1 ? '' : 's'}` : 'You are all caught up'}
          </p>
        </div>
        <div className="page-actions">
          <button className="btn btn-secondary" onClick={markAll} disabled={unread === 0}>
            <CheckCheck size={16} /> Mark all as read
          </button>
        </div>
      </div>

      <div className="tabs">
        {FILTERS.map((f) => {
          const count = f.id === 'all' ? all.length : all.filter((n) => n.type === f.id).length
          return (
            <button
              key={f.id}
              className={`tab ${filter === f.id ? 'active' : ''}`}
              onClick={() => setFilter(f.id)}
            >
              {f.label}
              <span className="count">{count}</span>
            </button>
          )
        })}
      </div>

      <div className="card">
        {rows.length === 0 ? (
          <EmptyState
            icon={Bell}
            title="No notifications here"
            message="Reminders about due dates, fines and reservations will appear in this list."
          />
        ) : (
          rows.map((n) => (
            <NotificationItem key={n.id} notification={n} onRead={markNotificationRead} />
          ))
        )}
      </div>
    </>
  )
}
