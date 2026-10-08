import { useRef } from 'react'
import { Link } from 'react-router-dom'
import { X, CheckCheck, Bell } from 'lucide-react'
import NotificationItem from '../notifications/NotificationItem.jsx'
import EmptyState from '../ui/EmptyState.jsx'
import { useLibrary } from '../../store/LibraryContext.jsx'
import { notificationsOf } from '../../utils/selectors.js'
import { useFocusTrap } from '../../hooks/useFocusTrap.js'

/**
 * Right-hand slide-over showing the latest notifications.
 * Opened from the bell in the topbar; Escape or a backdrop click closes it.
 */
export default function NotificationPanel({ open, onClose, notificationsPath, userId }) {
  const { state, markNotificationRead, markAllNotificationsRead } = useLibrary()
  const panelRef = useRef(null)
  const notifications = notificationsOf(state, userId)

  useFocusTrap(panelRef, open, onClose)

  if (!open) return null

  const unread = notifications.filter((n) => !n.read).length

  return (
    <div
      className="notif-overlay no-print"
      onMouseDown={(e) => {
        if (e.target === e.currentTarget) onClose()
      }}
    >
      <aside
        className="notif-panel"
        role="dialog"
        aria-modal="true"
        aria-label="Notifications"
        ref={panelRef}
      >
        <header className="notif-panel-head">
          <div>
            <h3>Notifications</h3>
            <p className="text-xs text-muted">{unread > 0 ? `${unread} unread` : 'All caught up'}</p>
          </div>
          <div className="row" style={{ gap: 6 }}>
            <button
              className="btn btn-ghost btn-sm"
              onClick={() => markAllNotificationsRead(userId)}
              disabled={unread === 0}
            >
              <CheckCheck size={14} /> Mark all
            </button>
            <button className="icon-btn" onClick={onClose} aria-label="Close notifications">
              <X size={17} />
            </button>
          </div>
        </header>

        <div className="notif-panel-body">
          {notifications.length === 0 ? (
            <EmptyState
              icon={Bell}
              title="No notifications"
              message="Due dates, reservations and announcements will appear here."
            />
          ) : (
            notifications
              .slice(0, 12)
              .map((n) => (
                <NotificationItem key={n.id} notification={n} onRead={markNotificationRead} />
              ))
          )}
        </div>

        <footer className="notif-panel-foot">
          <Link className="btn btn-secondary btn-block" to={notificationsPath} onClick={onClose}>
            View all notifications
          </Link>
        </footer>
      </aside>
    </div>
  )
}
