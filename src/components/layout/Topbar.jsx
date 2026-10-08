import { useEffect, useRef, useState } from 'react'
import { Link, useNavigate } from 'react-router-dom'
import {
  Menu,
  Bell,
  Sun,
  Moon,
  ChevronDown,
  UserRound,
  LogOut,
  Search,
  Megaphone,
} from 'lucide-react'
import { useAuth } from '../../store/AuthContext.jsx'
import { useTheme } from '../../store/ThemeContext.jsx'
import { getInitials } from '../../utils/format.js'

/** Sticky top bar: menu/back controls, search, theme toggle, bell, user menu. */
export default function Topbar({ onMenuClick, onBellClick, onAnnounceClick, unread, syncing = 0, notificationsPath, profilePath }) {
  const { user, logout } = useAuth()
  const { theme, toggle } = useTheme()
  const navigate = useNavigate()
  const [menuOpen, setMenuOpen] = useState(false)
  const menuRef = useRef(null)

  useEffect(() => {
    if (!menuOpen) return
    function onDocClick(e) {
      if (menuRef.current && !menuRef.current.contains(e.target)) setMenuOpen(false)
    }
    document.addEventListener('mousedown', onDocClick)
    return () => document.removeEventListener('mousedown', onDocClick)
  }, [menuOpen])

  const roleLabel = user?.role === 'admin' ? user?.title || 'Librarian' : user?.studentId

  function goProfile() {
    setMenuOpen(false)
    navigate(profilePath)
  }

  return (
    <header className="topbar">
      <button className="icon-btn menu-btn" onClick={onMenuClick} aria-label="Open navigation">
        <Menu size={19} />
      </button>

      <div className="topbar-right">
        {syncing > 0 && (
          <span className="sync-pill" role="status">
            <span className="sync-dot" aria-hidden="true" />
            Syncing…
          </span>
        )}

        <button
          className="cmdk-pill hide-sm"
          onClick={() => window.dispatchEvent(new CustomEvent('lms:open-cmdk'))}
          aria-label="Search pages and actions"
          data-tip="Search (Ctrl+K)"
        >
          <Search size={15} aria-hidden="true" />
          <span>Search…</span>
          <kbd>Ctrl K</kbd>
        </button>

        <button
          className="icon-btn"
          onClick={toggle}
          aria-label={theme === 'dark' ? 'Switch to light theme' : 'Switch to dark theme'}
          data-tip={theme === 'dark' ? 'Light mode' : 'Dark mode'}
        >
          {theme === 'dark' ? <Sun size={17} /> : <Moon size={17} />}
        </button>

        {user?.role === 'admin' && onAnnounceClick && (
          <button
            className="icon-btn"
            onClick={onAnnounceClick}
            aria-label="Make an announcement"
            aria-haspopup="dialog"
            data-tip="Announce"
          >
            <Megaphone size={17} />
          </button>
        )}

        <button
          className="icon-btn"
          onClick={onBellClick || (() => navigate(notificationsPath))}
          aria-label={`Notifications${unread ? `, ${unread} unread` : ''}`}
          aria-haspopup="dialog"
          data-tip="Notifications"
        >
          <Bell size={18} />
          {unread > 0 && <span className="dot" />}
        </button>

        <div className="user-menu-wrap" ref={menuRef}>
          <button
            className="topbar-user"
            onClick={() => setMenuOpen((open) => !open)}
            aria-haspopup="menu"
            aria-expanded={menuOpen}
          >
            <div className="avatar avatar-sm">{getInitials(user?.name)}</div>
            <span className="hide-sm" style={{ display: 'flex', flexDirection: 'column', alignItems: 'flex-start' }}>
              <span className="user-name">{user?.name}</span>
              <span className="user-role">{roleLabel}</span>
            </span>
            <ChevronDown size={15} className="text-muted" />
          </button>

          {menuOpen && (
            <div className="user-menu" role="menu">
              <div style={{ padding: '8px 11px' }}>
                <div className="font-semibold text-sm">{user?.name}</div>
                <div className="text-xs text-muted">{user?.email}</div>
              </div>
              <div className="menu-divider" />
              <button onClick={goProfile} role="menuitem">
                <UserRound size={16} /> My Profile
              </button>
              <Link to={notificationsPath} onClick={() => setMenuOpen(false)} role="menuitem">
                <Bell size={16} /> Notifications
                {unread > 0 && <span className="badge badge-primary" style={{ marginLeft: 'auto' }}>{unread}</span>}
              </Link>
              <div className="menu-divider" />
              <button
                className="danger"
                onClick={() => {
                  setMenuOpen(false)
                  logout()
                }}
                role="menuitem"
              >
                <LogOut size={16} /> Log out
              </button>
            </div>
          )}
        </div>
      </div>
    </header>
  )
}
