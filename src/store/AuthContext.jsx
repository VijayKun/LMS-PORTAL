import { createContext, useContext, useEffect, useState } from 'react'
import { loginRequest as mockLoginRequest } from '../services/api.js'
import {
  isOfflineError,
  login as backendLogin,
  logout as backendLogout,
  me as backendMe,
} from '../services/backend.js'

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

/**
 * Only these fields may be changed by the client after sign-in.
 * `role`, `id` etc. must come from the backend only - a forged session
 * can never escalate privileges through updateUser().
 */
const UPDATABLE_FIELDS = ['name', 'email', 'contact', 'avatar']

function isValidSession(parsed) {
  return Boolean(
    parsed &&
      typeof parsed === 'object' &&
      typeof parsed.id === 'string' &&
      typeof parsed.name === 'string' &&
      VALID_ROLES.includes(parsed.role),
  )
}

function readSavedUser() {
  try {
    const session = sessionStorage.getItem(STORAGE_KEY)
    if (session) {
      const parsed = JSON.parse(session)
      if (isValidSession(parsed)) return parsed
      sessionStorage.removeItem(STORAGE_KEY)
      return null
    }
    const legacy = localStorage.getItem(STORAGE_KEY)
    if (legacy) {
      const parsed = JSON.parse(legacy)
      localStorage.removeItem(STORAGE_KEY)
      localStorage.setItem(LEGACY_STORAGE_KEY, legacy)
      if (isValidSession(parsed)) return parsed
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
  const [backendAvailable, setBackendAvailable] = useState(null)

  useEffect(() => {
    try {
      if (user) sessionStorage.setItem(STORAGE_KEY, JSON.stringify(user))
      else sessionStorage.removeItem(STORAGE_KEY)
    } catch {
      /* session storage unavailable - in-memory session still works */
    }
  }, [user])

  /**
   * On a fresh tab, verify the HttpOnly session cookie with /api/auth/me.
   * Demo sessions intentionally skip this (there is no server cookie).
   */
  useEffect(() => {
    let cancelled = false
    if (!user) {
      setBackendAvailable(null)
      return undefined
    }
    if (user.demo) {
      setBackendAvailable(false)
      return undefined
    }

    ;(async () => {
      try {
        const fresh = await backendMe()
        if (cancelled) return
        if (fresh) {
          setUser((current) => (current ? { ...current, ...fresh, demo: false } : current))
          setBackendAvailable(true)
        }
      } catch (err) {
        if (cancelled) return
        if (err.status === 401) {
          setUser(null)
          setBackendAvailable(true)
        } else if (isOfflineError(err)) {
          setBackendAvailable(false)
        } else {
          setBackendAvailable(true)
        }
      }
    })()

    return () => {
      cancelled = true
    }
    // This should run once per saved session, not on every user object change.
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [])

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
      const loggedUser = await backendLogin(id, password)
      setUser({ ...loggedUser, demo: false })
      setBackendAvailable(true)
      return loggedUser
    } catch (err) {
      if (err.status === 401 || err.code === 'invalid_credentials') {
        setBackendAvailable(true)
        setError('Invalid email or password.')
        return false
      }

      if (isOfflineError(err)) {
        // Keep the demo usable when the C backend / database is offline.
        try {
          const demoUser = await mockLoginRequest({ email: id, role })
          const withDemoFlag = { ...demoUser, demo: true }
          setUser(withDemoFlag)
          setBackendAvailable(false)
          return withDemoFlag
        } catch {
          setError('Login failed. Please try again.')
          return false
        }
      }

      setBackendAvailable(true)
      setError(err.message || 'Login failed. Please try again.')
      return false
    } finally {
      setLoading(false)
    }
  }

  function logout() {
    setUser(null)
    setBackendAvailable(null)
    backendLogout()
    try {
      sessionStorage.removeItem(STORAGE_KEY)
      localStorage.removeItem(STORAGE_KEY)
      // Remove the legacy localStorage session so it can never be revived.
      localStorage.removeItem(LEGACY_STORAGE_KEY)
    } catch {
      /* storage unavailable - in-memory state is already cleared */
    }
  }

  /** Used by the profile page / settings page. Whitelisted - never role/id. */
  function updateUser(patch) {
    setUser((current) => {
      if (!current) return current
      const safe = {}
      for (const key of UPDATABLE_FIELDS) {
        if (key in patch) safe[key] = patch[key]
      }
      return { ...current, ...safe }
    })
  }

  return (
    <AuthContext.Provider
      value={{
        user,
        loading,
        error,
        login,
        logout,
        updateUser,
        setError,
        backendAvailable,
      }}
    >
      {children}
    </AuthContext.Provider>
  )
}

export function useAuth() {
  const ctx = useContext(AuthContext)
  if (!ctx) throw new Error('useAuth must be used inside <AuthProvider>')
  return ctx
}

/** Safe variant for providers that are tested/rendered without an AuthProvider. */
export function useAuthOptional() {
  return useContext(AuthContext)
}
