import { useState } from 'react'
import { Megaphone, X } from 'lucide-react'
import { useLibrary } from '../../store/LibraryContext.jsx'
import { useAuth } from '../../store/AuthContext.jsx'

const DISMISS_KEY = 'lms.announce.dismissed'

/**
 * Glowing banner for the newest library announcement. Shown at the top of
 * every portal page until the reader dismisses it (dismissal is remembered
 * per announcement id, so a brand-new announcement shows up again).
 */
export default function AnnouncementBanner() {
  const { state } = useLibrary()
  const { user } = useAuth()

  const latest = state.notifications.find(
    (n) =>
      n.type === 'announcement' &&
      (n.userId === 'all' || n.userId === user?.id),
  )

  const [dismissed, setDismissed] = useState(() => {
    try {
      return localStorage.getItem(DISMISS_KEY) || ''
    } catch {
      return ''
    }
  })

  if (!latest || dismissed === latest.id) return null

  function dismiss() {
    setDismissed(latest.id)
    try {
      localStorage.setItem(DISMISS_KEY, latest.id)
    } catch {
      /* storage unavailable - banner stays dismissed for this session */
    }
  }

  return (
    <aside className="announce-banner no-print" role="status">
      <div className="announce-icon">
        <Megaphone size={19} />
      </div>
      <div className="announce-body">
        <div className="announce-kicker">Announcement</div>
        <div className="announce-title">{latest.title}</div>
        <div className="announce-msg">{latest.message}</div>
      </div>
      <button className="icon-btn announce-close" onClick={dismiss} aria-label="Dismiss announcement">
        <X size={17} />
      </button>
    </aside>
  )
}
