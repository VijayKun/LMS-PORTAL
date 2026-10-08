import { BookOpen, CalendarClock, AlertTriangle, BellRing, Megaphone, Check } from 'lucide-react'
import { relativeDay } from '../../utils/format.js'

const TYPES = {
  due: { icon: CalendarClock, tone: 'var(--warning-soft)', color: 'var(--warning)' },
  overdue: { icon: AlertTriangle, tone: 'var(--danger-soft)', color: 'var(--danger)' },
  fine: { icon: AlertTriangle, tone: 'var(--danger-soft)', color: 'var(--danger)' },
  reservation: { icon: BookOpen, tone: 'var(--primary-soft)', color: 'var(--primary)' },
  announcement: { icon: Megaphone, tone: 'var(--info-soft)', color: 'var(--info)' },
}

/** Single row in the notification centre. */
export default function NotificationItem({ notification, onRead }) {
  const config = TYPES[notification.type] || TYPES.announcement
  const Icon = config.icon

  return (
    <div className={`notif-item ${notification.read ? '' : 'unread'}`}>
      <div className="notif-icon" style={{ background: config.tone, color: config.color }}>
        <Icon size={17} />
      </div>
      <div className="notif-body">
        <div className="notif-title">{notification.title}</div>
        <div className="notif-message">{notification.message}</div>
        <div className="notif-time">{relativeDay(notification.time)}</div>
      </div>
      {!notification.read ? (
        <>
          <span className="notif-unread-dot" title="Unread" />
          <button
            className="btn btn-ghost btn-sm"
            onClick={() => onRead(notification.id)}
            aria-label={`Mark "${notification.title}" as read`}
          >
            <Check size={14} /> Mark read
          </button>
        </>
      ) : (
        <span className="text-xs text-muted nowrap">Read</span>
      )}
    </div>
  )
}
