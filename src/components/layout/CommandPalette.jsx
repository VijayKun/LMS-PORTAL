import { useEffect, useMemo, useRef, useState } from 'react'
import { useNavigate } from 'react-router-dom'
import { Search, Moon, Sun, Bell, LogOut, CornerDownLeft } from 'lucide-react'
import { useTheme } from '../../store/ThemeContext.jsx'
import { useAuth } from '../../store/AuthContext.jsx'
import { useFocusTrap } from '../../hooks/useFocusTrap.js'

/**
 * Ctrl+K command palette: jump to any page, toggle theme, open
 * notifications or log out. Opens via the keyboard shortcut or the
 * search pill in the topbar (custom `lms:open-cmdk` event).
 */
export default function CommandPalette({ sections, notificationsPath }) {
  const [open, setOpen] = useState(false)
  const [query, setQuery] = useState('')
  const [active, setActive] = useState(0)
  const inputRef = useRef(null)
  const dialogRef = useRef(null)
  const openRef = useRef(false)
  const navigate = useNavigate()
  const { theme, toggle } = useTheme()
  const { logout } = useAuth()

  useEffect(() => {
    openRef.current = open
  }, [open])

  useFocusTrap(dialogRef, open, () => setOpen(false))

  const items = useMemo(() => {
    const nav = sections.flatMap((section) =>
      section.links.map((link) => ({
        id: `nav-${link.to}`,
        label: link.label,
        hint: section.label,
        icon: link.icon,
        run: () => navigate(link.to),
      })),
    )
    const actions = [
      {
        id: 'theme',
        label: theme === 'dark' ? 'Switch to light mode' : 'Switch to dark mode',
        hint: 'Action',
        icon: theme === 'dark' ? Sun : Moon,
        run: toggle,
      },
      { id: 'notif', label: 'Open notifications', hint: 'Action', icon: Bell, run: () => navigate(notificationsPath) },
      { id: 'logout', label: 'Log out', hint: 'Action', icon: LogOut, run: logout },
    ]
    return [...nav, ...actions]
  }, [sections, theme, navigate, logout, notificationsPath, toggle])

  const filtered = useMemo(() => {
    const q = query.trim().toLowerCase()
    if (!q) return items
    return items.filter(
      (item) => item.label.toLowerCase().includes(q) || item.hint.toLowerCase().includes(q),
    )
  }, [items, query])

  // keyboard shortcut (Ctrl/Cmd + K) + topbar pill
  useEffect(() => {
    function open() {
      setQuery('')
      setActive(0)
      setOpen(true)
    }
    // "g then key" quick-jump chords, per panel.
    const chord = { key: null, at: 0 }
    const studentMap = {
      d: '/student',
      b: '/student/books',
      m: '/student/my-books',
      r: '/student/reservations',
      h: '/student/history',
      f: '/student/fines',
      n: '/student/notifications',
      p: '/student/profile',
    }
    const adminMap = {
      d: '/admin',
      b: '/admin/books',
      p: '/admin/students',
      t: '/admin/transactions',
      i: '/admin/import',
      r: '/admin/reservations',
      f: '/admin/fines',
      n: '/admin/notifications',
      e: '/admin/reports',
      a: '/admin/analytics',
      s: '/admin/settings',
    }
    const map = notificationsPath.startsWith('/admin') ? adminMap : studentMap

    function onKey(e) {
      if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'k') {
        e.preventDefault()
        if (openRef.current) setOpen(false)
        else open()
        return
      }
      if (e.ctrlKey || e.metaKey || e.altKey || openRef.current) return
      const tag = e.target?.tagName
      if (tag === 'INPUT' || tag === 'TEXTAREA' || tag === 'SELECT' || e.target?.isContentEditable) return

      const key = e.key.toLowerCase()
      const now = Date.now()
      if (chord.key === 'g' && now - chord.at < 900) {
        chord.key = null
        const to = map[key]
        if (to) {
          e.preventDefault()
          navigate(to)
        }
        return
      }
      if (key === 'g') {
        chord.key = 'g'
        chord.at = now
        return
      }
      if (e.key === '?') {
        e.preventDefault()
        open()
      }
    }
    window.addEventListener('keydown', onKey)
    window.addEventListener('lms:open-cmdk', open)
    return () => {
      window.removeEventListener('keydown', onKey)
      window.removeEventListener('lms:open-cmdk', open)
    }
  }, [notificationsPath, navigate])

  // move focus into the input once the dialog mounts
  useEffect(() => {
    if (open) requestAnimationFrame(() => inputRef.current?.focus())
  }, [open])

  if (!open) return null

  function run(item) {
    setOpen(false)
    item.run()
  }

  function onKeyDown(e) {
    if (e.key === 'Escape') setOpen(false)
    else if (e.key === 'ArrowDown') {
      e.preventDefault()
      setActive((current) => Math.min(current + 1, filtered.length - 1))
    } else if (e.key === 'ArrowUp') {
      e.preventDefault()
      setActive((current) => Math.max(current - 1, 0))
    } else if (e.key === 'Enter') {
      e.preventDefault()
      const item = filtered[active]
      if (item) run(item)
    }
  }

  return (
    <div
      className="cmdk-overlay no-print"
      onMouseDown={(e) => {
        if (e.target === e.currentTarget) setOpen(false)
      }}
    >
      <div className="cmdk" role="dialog" aria-modal="true" aria-label="Command palette" ref={dialogRef}>
        <div className="cmdk-input">
          <Search size={16} aria-hidden="true" />
          <input
            ref={inputRef}
            value={query}
            onChange={(e) => {
              setQuery(e.target.value)
              setActive(0)
            }}
            onKeyDown={onKeyDown}
            placeholder="Search pages and actions…"
            aria-label="Search pages and actions"
          />
          <kbd>Esc</kbd>
        </div>
        <div className="cmdk-list" role="listbox" aria-label="Results">
          {filtered.length === 0 && <div className="cmdk-empty">No matches for “{query}”</div>}
          {filtered.map((item, index) => (
            <button
              key={item.id}
              className={`cmdk-item ${index === active ? 'active' : ''}`}
              role="option"
              aria-selected={index === active}
              onMouseEnter={() => setActive(index)}
              onClick={() => run(item)}
            >
              <item.icon size={16} aria-hidden="true" />
              <span className="cmdk-label">{item.label}</span>
              <span className="cmdk-hint">{item.hint}</span>
              {index === active && <CornerDownLeft size={13} className="cmdk-enter" aria-hidden="true" />}
            </button>
          ))}
        </div>
        <div className="cmdk-footer">
          <span><kbd>↑</kbd><kbd>↓</kbd> navigate</span>
          <span><kbd>↵</kbd> open</span>
          <span><kbd>Ctrl</kbd><kbd>K</kbd> toggle</span>
          <span><kbd>G</kbd> <kbd>B</kbd> quick jump</span>
        </div>
      </div>
    </div>
  )
}
