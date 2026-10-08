import { useMemo, useState } from 'react'
import { Bell, CheckCheck, Megaphone } from 'lucide-react'
import { useLibrary } from '../../store/LibraryContext.jsx'
import { useAuth } from '../../store/AuthContext.jsx'
import { usePageTitle } from '../../store/PageTitleContext.jsx'
import { useToast } from '../../store/ToastContext.jsx'
import NotificationItem from '../../components/notifications/NotificationItem.jsx'
import EmptyState from '../../components/ui/EmptyState.jsx'
import Modal from '../../components/ui/Modal.jsx'
import { Field, Input, Textarea } from '../../components/ui/Field.jsx'
import { notificationsOf } from '../../utils/selectors.js'

const FILTERS = [
  { id: 'all', label: 'All' },
  { id: 'overdue', label: 'Overdue' },
  { id: 'announcement', label: 'Announcements' },
  { id: 'due', label: 'Due Reminders' },
  { id: 'fine', label: 'Fines' },
  { id: 'reservation', label: 'Reservations' },
]

export default function AdminNotifications() {
  usePageTitle('Notifications')
  const { state, markNotificationRead, markAllNotificationsRead, announce } = useLibrary()
  const { user } = useAuth()
  const { toast } = useToast()

  const [filter, setFilter] = useState('all')
  const [posting, setPosting] = useState(false)
  const [title, setTitle] = useState('')
  const [message, setMessage] = useState('')
  const [errors, setErrors] = useState({})
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

  function openPost() {
    setTitle('')
    setMessage('')
    setErrors({})
    setPosting(true)
  }

  function postAnnouncement(e) {
    e.preventDefault()
    const errs = {}
    if (!title.trim()) errs.title = 'Title is required.'
    if (!message.trim()) errs.message = 'Message is required.'
    setErrors(errs)
    if (Object.keys(errs).length) return

    announce(title.trim(), message.trim())
    setPosting(false)
    toast('Announcement published to everyone.', 'success')
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
          <button className="btn btn-primary" onClick={openPost}>
            <Megaphone size={16} /> New announcement
          </button>
          <button className="btn btn-secondary" onClick={markAll} disabled={unread === 0}>
            <CheckCheck size={16} /> Mark all as read
          </button>
        </div>
      </div>

      <div className="tabs">
        {FILTERS.map((f) => {
          const count = f.id === 'all' ? all.length : all.filter((n) => n.type === f.id).length
          return (
            <button key={f.id} className={`tab ${filter === f.id ? 'active' : ''}`} onClick={() => setFilter(f.id)}>
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
            title="No notifications"
            message="System alerts and announcements will appear here."
          />
        ) : (
          rows.map((n) => (
            <NotificationItem key={n.id} notification={n} onRead={markNotificationRead} />
          ))
        )}
      </div>

      <Modal
        open={posting}
        title="New announcement"
        onClose={() => setPosting(false)}
        footer={
          <>
            <button className="btn btn-secondary" onClick={() => setPosting(false)}>
              Cancel
            </button>
            <button className="btn btn-primary" onClick={postAnnouncement}>
              <Megaphone size={15} /> Publish
            </button>
          </>
        }
      >
        <form className="stack stack-sm" onSubmit={postAnnouncement} noValidate>
          <Field label="Title" required error={errors.title} htmlFor="ann-title">
            <Input
              id="ann-title"
              value={title}
              error={errors.title}
              onChange={(e) => setTitle(e.target.value)}
              placeholder="e.g. Library closed on 18 October"
            />
          </Field>
          <Field label="Message" required error={errors.message} htmlFor="ann-msg">
            <Textarea
              id="ann-msg"
              rows={4}
              value={message}
              error={errors.message}
              onChange={(e) => setMessage(e.target.value)}
              placeholder="What should everyone know?"
            />
          </Field>
          <p className="text-xs text-muted">Shown as a glowing banner on every page for all users.</p>
        </form>
      </Modal>
    </>
  )
}
