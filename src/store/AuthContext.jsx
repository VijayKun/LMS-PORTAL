import { createContext, useContext, useEffect, useState } from 'react'
import { loginRequest } from '../services/api.js'

/**
 * Mock authentication.
 *
 *  - login() validates the form, calls the mock service and stores the user
 *  - the session is kept per tab in sessionStorage, so refreshing the page
 *    keeps you signed in as the same account without sharing the login with
 *    other open tabs
 *  - when the real backend arrives, only `loginRequest` / `logoutRequest`
 *    inside services/api.js have to be replaced with real endpoints
 */
const AuthContext = createContext(null)
const STORAGE_KEY = 'lms.auth.v1'

/**
 * Older builds stored the session in localStorage. Per-tab sessions live in
 * sessionStorage now; a legacy session is adopted once and then removed so a
 * second tab never picks it up.
 */
const LEGACY_STORAGE_KEY = 'lms.auth.v1.local'

/** College email domains per role (students may also sign in with a student ID). */
export const EMAIL_DOMAINS = {
  student: 'mitsgwl.ac.in',
  admin: 'mitsgwalior.in',
}

const VALID_ROLES = ['student', 'admin']

function readSavedUser() {
  try {
    const session = sessionStorage.getItem(STORAGE_KEY)
    if (session) {
      const parsed = JSON.parse(session)
      if (parsed && VALID_ROLES.includes(parsed.role)) return parsed
      sessionStorage.removeItem(STORAGE_KEY)
      return null
    }
    const legacy = localStorage.getItem(STORAGE_KEY)
    if (legacy) {
      const parsed = JSON.parse(legacy)
      localStorage.removeItem(STORAGE_KEY)
      localStorage.setItem(LEGACY_STORAGE_KEY, legacy)
      if (parsed && VALID_ROLES.includes(parsed.role)) return parsed
    }
  } catch {
    /* corrupted storage - just start signed out */
  }
  return null
}

export function AuthProvider({ children }) {
  const [user, setUser] = useState(readSavedUser)
  const [loading, setLoading] = useState(false)
  const [error, setError] = useState('')

  useEffect(() => {
    try {
      if (user) sessionStorage.setItem(STORAGE_KEY, JSON.stringify(user))
      else sessionStorage.removeItem(STORAGE_KEY)
    } catch {
      /* session storage unavailable - in-memory session still works */
    }
  }, [user])

  /**
   * Accepts a college email or a student ID (e.g. 2025CS1042).
   * The role is derived from the identifier, never chosen by the user:
   *   @mitsgwl.ac.in -> student (a bare student ID also means student)
   *   @mitsgwalior.in -> faculty / librarian
   */
  async function login(identifier, password) {
    setError('')
    const id = String(identifier || '').trim()

    if (!id) {
      setError('Enter your college email or student ID.')
      return false
    }

    let role
    if (id.includes('@')) {
      if (!/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(id)) {
        setError('Enter a valid email address.')
        return false
      }
      const domain = id.slice(id.lastIndexOf('@') + 1).toLowerCase()
      if (domain === EMAIL_DOMAINS.student) role = 'student'
      else if (domain === EMAIL_DOMAINS.admin) role = 'admin'
      else {
        setError(
          `Use your college email - students @${EMAIL_DOMAINS.student}, faculty & librarians @${EMAIL_DOMAINS.admin}.`,
        )
        return false
      }
    } else {
      role = 'student'
    }

    if (!password) {
      setError('Enter your password.')
      return false
    }
    if (password.length < 6) {
      setError('Password must be at least 6 characters.')
      return false
    }

    setLoading(true)
    try {
      const loggedUser = await loginRequest({ email: id, role })
      setUser(loggedUser)
      return loggedUser
    } catch {
      setError('Login failed. Please try again.')
      return false
    } finally {
      setLoading(false)
    }
  }

  function logout() {
    setUser(null)
  }

  /** Used by the profile page / settings page. */
  function updateUser(patch) {
    setUser((current) => (current ? { ...current, ...patch } : current))
  }

  return (
    <AuthContext.Provider value={{ user, loading, error, login, logout, updateUser, setError }}>
      {children}
    </AuthContext.Provider>
  )
}

export function useAuth() {
  const ctx = useContext(AuthContext)
  if (!ctx) throw new Error('useAuth must be used inside <AuthProvider>')
  return ctx
}
