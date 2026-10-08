import { useState } from 'react'
import { useNavigate, Navigate } from 'react-router-dom'
import { Eye, EyeOff, AlertCircle, Loader2, BookOpen, Search, ArrowLeftRight, Bookmark, IndianRupee, Users, Lock, Code2, Sun, Moon, ChevronDown } from 'lucide-react'
import { useAuth } from '../store/AuthContext.jsx'
import { useTheme } from '../store/ThemeContext.jsx'
import { useToast } from '../store/ToastContext.jsx'
import { Field, Input } from '../components/ui/Field.jsx'
import BrandLogo from '../components/BrandLogo.jsx'

const DEMO = {
  identifier: 'shivangi.verma@mitsgwl.ac.in',
  password: 'library123',
}

const FEATURES = [
  { icon: BookOpen, title: 'Book Management', desc: 'Manage and explore the complete library collection.' },
  { icon: Search, title: 'Smart Search', desc: 'Find books quickly by title, author, ISBN or category.' },
  { icon: ArrowLeftRight, title: 'Issue & Return', desc: 'Track borrowed books, returns and due dates.' },
  { icon: Bookmark, title: 'Reservations', desc: 'Reserve unavailable books and manage your queue.' },
  { icon: IndianRupee, title: 'Fine Management', desc: 'Track overdue books and library fines.' },
  { icon: Users, title: 'Student Management', desc: 'Manage student accounts and borrowing activity.' },
]

function GoogleIcon({ size = 18 }) {
  return (
    <svg width={size} height={size} viewBox="0 0 48 48" aria-hidden="true">
      <path fill="#EA4335" d="M24 9.5c3.54 0 6.71 1.22 9.21 3.6l6.85-6.85C35.9 2.38 30.47 0 24 0 14.62 0 6.51 5.38 2.56 13.22l7.98 6.19C12.43 13.72 17.74 9.5 24 9.5z" />
      <path fill="#4285F4" d="M46.98 24.55c0-1.57-.15-3.09-.38-4.55H24v9.02h12.94c-.58 2.96-2.26 5.48-4.78 7.18l7.73 6c4.51-4.18 7.09-10.36 7.09-17.65z" />
      <path fill="#FBBC05" d="M10.53 28.59c-.48-1.45-.76-2.99-.76-4.59s.27-3.14.76-4.59l-7.98-6.19C.92 16.46 0 20.12 0 24c0 3.88.92 7.54 2.56 10.78l7.97-6.19z" />
      <path fill="#34A853" d="M24 48c6.48 0 11.93-2.13 15.89-5.81l-7.73-6c-2.15 1.45-4.92 2.3-8.16 2.3-6.26 0-11.57-4.22-13.47-9.91l-7.98 6.19C6.51 42.62 14.62 48 24 48z" />
    </svg>
  )
}

export default function LoginPage() {
  const { user, login, loading, error, setError } = useAuth()
  const { theme, toggle } = useTheme()
  const { toast } = useToast()
  const navigate = useNavigate()

  const [identifier, setIdentifier] = useState(DEMO.identifier)
  const [password, setPassword] = useState(DEMO.password)
  const [showPassword, setShowPassword] = useState(false)
  const [capsOn, setCapsOn] = useState(false)
  const [remember, setRemember] = useState(true)
  const [featuresOpen, setFeaturesOpen] = useState(false)

  if (user) return <Navigate to={user.role === 'admin' ? '/admin' : '/student'} replace />

  const detectCaps = (e) => setCapsOn(Boolean(e.getModifierState?.('CapsLock')))

  async function handleSubmit(e) {
    e.preventDefault()
    const loggedIn = await login(identifier, password)
    if (loggedIn) {
      if (remember) localStorage.setItem('lms.remember', 'true')
      navigate(loggedIn.role === 'admin' ? '/admin' : '/student', { replace: true })
    }
  }

  /**
   * Google sign-in is a frontend demo for now.
   * When real OAuth is added, replace the toast below with a call to the
   * Google Identity flow (e.g. fetch a credential then `await login(...)`
   * with the returned user, mirroring handleSubmit's navigation).
   */
  function handleGoogleSignIn() {
    toast('Google Sign-In is currently in demo mode.', 'info')
  }

  return (
    <div className="login-page">
      <header className="login-header">
        <div className="login-header-brand">
          <div className="login-header-logo">
            <BrandLogo size={18} />
          </div>
          <span className="login-header-name">LMS</span>
          <span className="login-header-sub">Library Management System</span>
        </div>
        <div className="login-header-actions">
          <button
            type="button"
            className="dev-pill"
            onClick={() => toast('Developer profile is coming soon.', 'info')}
          >
            <Code2 size={13} /> Developer
          </button>
          <button
            type="button"
            className="header-theme"
            onClick={toggle}
            aria-label={theme === 'dark' ? 'Switch to light theme' : 'Switch to dark theme'}
          >
            {theme === 'dark' ? <Moon size={15} /> : <Sun size={15} />}
          </button>
        </div>
      </header>

      <div className="login-split">
        <aside className="login-aside">
          <div className="aside-glow" aria-hidden="true" />
          <div className="aside-inner">
            <div className="aside-brand">
              <div className="aside-logo">
                <BrandLogo size={30} />
              </div>
              <div className="aside-kicker">LMS</div>
              <h2 className="aside-title">Library Management System</h2>
              <p className="aside-institute">
                Madhav Institute of Science &amp; Technology, Gwalior
              </p>
            </div>
          </div>

          <section className="feature-section">
            <h3 className="feature-heading">Library Services</h3>
            <button
              type="button"
              className="feature-toggle"
              aria-expanded={featuresOpen}
              aria-controls="library-services"
              onClick={() => setFeaturesOpen((v) => !v)}
            >
              {featuresOpen ? 'See Less' : 'See More'}
              <ChevronDown size={16} />
            </button>
            <div id="library-services" className={`feature-collapse${featuresOpen ? ' open' : ''}`}>
              <div className="feature-grid">
                {FEATURES.map(({ icon: Icon, title, desc }, i) => (
                  <div className="feature-card" key={title} style={{ animationDelay: `${i * 60}ms` }}>
                    <div className="feature-icon">
                      <Icon size={16} />
                    </div>
                    <div className="feature-title">{title}</div>
                    <div className="feature-desc">{desc}</div>
                  </div>
                ))}
              </div>
            </div>
          </section>
        </aside>

        <main className="login-main">
          <div className="login-card">
            <div className="login-lock">
              <Lock size={19} />
            </div>
            <h1>Sign in to your account</h1>
            <p className="login-sub">Login with Institute email:</p>

            <div className="domain-hints">
              <span className="domain-chip">
                <b>@mitsgwalior.in</b>
                <i>Librarian</i>
              </span>
              <span className="domain-chip">
                <b>@mitsgwl.ac.in</b>
                <i>Students</i>
              </span>
            </div>

            <button
              type="button"
              className="google-btn"
              onClick={handleGoogleSignIn}
            >
              <GoogleIcon />
              Sign in with Google
            </button>

            <div className="or-divider">
              <span>or continue with credentials</span>
            </div>

            {error && (
              <div className="login-alert" role="alert">
                <AlertCircle size={16} />
                <span>{error}</span>
              </div>
            )}

            <form onSubmit={handleSubmit} noValidate>
              <Field label="College email or Student ID" required htmlFor="identifier">
                <Input
                  id="identifier"
                  name="identifier"
                  autoComplete="username"
                  placeholder="name@mitsgwl.ac.in"
                  value={identifier}
                  onChange={(e) => {
                    setIdentifier(e.target.value)
                    setError('')
                  }}
                />
              </Field>

              <Field label="Password" required htmlFor="password">
                <div className="pw-wrap">
                  <Input
                    id="password"
                    name="password"
                    type={showPassword ? 'text' : 'password'}
                    autoComplete="current-password"
                    placeholder="Minimum 6 characters"
                    value={password}
                    onChange={(e) => {
                      setPassword(e.target.value)
                      setError('')
                    }}
                    onKeyUp={detectCaps}
                    onKeyDown={detectCaps}
                    onBlur={() => setCapsOn(false)}
                  />
                  <button
                    type="button"
                    className="pw-toggle"
                    onClick={() => setShowPassword((v) => !v)}
                    aria-label={showPassword ? 'Hide password' : 'Show password'}
                    aria-pressed={showPassword}
                  >
                    {showPassword ? <EyeOff size={16} /> : <Eye size={16} />}
                  </button>
                </div>
                {capsOn && (
                  <p className="caps-warn">
                    <AlertCircle size={12} /> Caps Lock is on
                  </p>
                )}
              </Field>

              <div className="login-row">
                <label className="remember">
                  <input type="checkbox" checked={remember} onChange={(e) => setRemember(e.target.checked)} />
                  Remember me
                </label>
                <button
                  type="button"
                  className="link-btn"
                  onClick={() => setError('Password reset is not available in this frontend demo.')}
                >
                  Forgot password?
                </button>
              </div>

              <button className="btn btn-primary btn-lg btn-block" type="submit" disabled={loading}>
                {loading && <Loader2 size={17} className="spin" />}
                {loading ? 'Signing in…' : 'Sign In'}
              </button>
            </form>

            <p className="demo-note">Demo mode · credentials are pre-filled.</p>
          </div>
        </main>
      </div>
    </div>
  )
}