import { createContext, useContext, useEffect, useState } from 'react'

/**
 * Light / dark theme toggle for the whole app.
 *
 * The chosen MODE is persisted in localStorage and can be "light", "dark" or
 * "system" (follow the OS). The resolved theme is applied as
 * `data-theme="dark"` on <html>, which switches the CSS custom properties
 * defined in src/index.css. `toggle()` flips to the explicit opposite, so the
 * topbar button keeps working.
 *
 * The interface density preference ("comfortable"/"compact") lives here too
 * and is applied as `data-density` on <html>.
 */
const ThemeContext = createContext(null)
const MODE_KEY = 'lms.theme'
const DENSITY_KEY = 'lms.density'

const validMode = (value) => (value === 'light' || value === 'dark' || value === 'system' ? value : null)

function readStored(key) {
  try {
    return localStorage.getItem(key)
  } catch {
    return null
  }
}

function systemPrefersDark() {
  return typeof window !== 'undefined' && window.matchMedia?.('(prefers-color-scheme: dark)')?.matches
}

export function ThemeProvider({ children }) {
  const [mode, setMode] = useState(() => validMode(readStored(MODE_KEY)) ?? 'light')
  const [density, setDensity] = useState(() => (readStored(DENSITY_KEY) === 'compact' ? 'compact' : 'comfortable'))
  const [systemDark, setSystemDark] = useState(systemPrefersDark)

  // Keep the resolved theme in sync when the OS preference changes.
  useEffect(() => {
    const mq = window.matchMedia?.('(prefers-color-scheme: dark)')
    if (!mq) return undefined
    const handler = (event) => setSystemDark(event.matches)
    mq.addEventListener?.('change', handler)
    return () => mq.removeEventListener?.('change', handler)
  }, [])

  const theme = mode === 'system' ? (systemDark ? 'dark' : 'light') : mode

  useEffect(() => {
    document.documentElement.dataset.theme = theme
  }, [theme])

  useEffect(() => {
    document.documentElement.dataset.density = density
  }, [density])

  useEffect(() => {
    try {
      localStorage.setItem(MODE_KEY, mode)
    } catch {
      /* storage unavailable - preferences still apply for this session */
    }
  }, [mode])

  useEffect(() => {
    try {
      localStorage.setItem(DENSITY_KEY, density)
    } catch {
      /* storage unavailable */
    }
  }, [density])

  /** Topbar quick toggle: take the current theme and make it explicit. */
  const toggle = () => {
    const next = theme === 'dark' ? 'light' : 'dark'
    setMode(next)
    return next
  }

  return (
    <ThemeContext.Provider value={{ theme, mode, setMode, toggle, density, setDensity }}>
      {children}
    </ThemeContext.Provider>
  )
}

export function useTheme() {
  const ctx = useContext(ThemeContext)
  if (!ctx) throw new Error('useTheme must be used inside <ThemeProvider>')
  return ctx
}