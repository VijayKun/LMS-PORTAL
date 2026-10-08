import { useRef, useState } from 'react'
import { Link } from 'react-router-dom'
import { X, Megaphone } from 'lucide-react'
import { Field, Input, Textarea } from '../ui/Field.jsx'
import { useLibrary } from '../../store/LibraryContext.jsx'
import { useToast } from '../../store/ToastContext.jsx'
import { useFocusTrap } from '../../hooks/useFocusTrap.js'

/**
 * Right-hand slide-over with the "Make an Announcement" form.
 * Opened from the megaphone button in the topbar (admin only);
 * Escape or a backdrop click closes it.
 */
export default function AnnouncementPanel({ open, onClose, notificationsPath }) {
  const { announce } = useLibrary()
  const { toast } = useToast()
  const panelRef = useRef(null)
  const [title, setTitle] = useState('')
  const [message, setMessage] = useState('')
  const [errors, setErrors] = useState({})

  useFocusTrap(panelRef, open, onClose)

  if (!open) return null

  function publish(e) {
    e.preventDefault()
    const errs = {}
    if (!title.trim()) errs.title = 'Title is required.'
    if (!message.trim()) errs.message = 'Message is required.'
    setErrors(errs)
    if (Object.keys(errs).length) return

    announce(title.trim(), message.trim())
    setTitle('')
    setMessage('')
    setErrors({})
    toast('Announcement published to everyone.', 'success')
    onClose()
  }

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
        aria-label="Make an announcement"
        ref={panelRef}
      >
        <header className="notif-panel-head">
          <div>
            <h3>
              <span className="card-ico primary"><Megaphone size={15} /></span>
              Make an Announcement
            </h3>
            <p className="text-xs text-muted">Broadcasts a banner to every user.</p>
          </div>
          <button className="icon-btn" onClick={onClose} aria-label="Close announcement panel">
            <X size={17} />
          </button>
        </header>

        <div className="notif-panel-body">
          <form className="stack stack-sm ann-panel-form" onSubmit={publish} noValidate>
            <Field label="Title" required error={errors.title} htmlFor="panel-ann-title">
              <Input
                id="panel-ann-title"
                value={title}
                error={errors.title}
                onChange={(e) => setTitle(e.target.value)}
                maxLength={80}
                placeholder="e.g. Library timings extended during exams"
                autoFocus
              />
            </Field>
            <Field label="Message" required error={errors.message} htmlFor="panel-ann-msg">
              <Textarea
                id="panel-ann-msg"
                rows={5}
                value={message}
                error={errors.message}
                onChange={(e) => setMessage(e.target.value)}
                maxLength={300}
                placeholder="What should every reader know?"
              />
            </Field>
            <div className="row-between">
              <span className="text-xs text-muted char-count">{message.length}/300</span>
              <button className="btn btn-primary" type="submit">
                <Megaphone size={15} /> Publish
              </button>
            </div>
          </form>
        </div>

        <footer className="notif-panel-foot">
          <Link className="btn btn-secondary btn-block" to={notificationsPath} onClick={onClose}>
            Open notification centre
          </Link>
        </footer>
      </aside>
    </div>
  )
}
