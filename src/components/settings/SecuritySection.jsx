import { useState } from 'react'
import { Monitor, Smartphone, LogOut, KeyRound, ShieldCheck, Clock } from 'lucide-react'
import { useToast } from '../../store/ToastContext.jsx'
import { Field, Input } from '../ui/Field.jsx'
import ConfirmDialog from '../ui/ConfirmDialog.jsx'
import SectionHead from './SectionHead.jsx'

const MOCK_SESSIONS = [
  {
    id: 's1',
    device: 'Chrome · Windows 11',
    location: 'Gwalior, India',
    lastActive: 'Active now',
    icon: Monitor,
    current: true,
  },
  {
    id: 's2',
    device: 'Safari · iPhone 15',
    location: 'Gwalior, India',
    lastActive: '2 hours ago',
    icon: Smartphone,
    current: false,
  },
  {
    id: 's3',
    device: 'Firefox · MacBook Air',
    location: 'Indore, India',
    lastActive: 'Yesterday, 21:04',
    icon: Monitor,
    current: false,
  },
]

const MOCK_ACTIVITY = [
  { id: 'a1', action: 'Signed in', detail: 'Chrome · Windows 11', time: 'Today, 09:14' },
  { id: 'a2', action: 'Changed password', detail: 'Chrome · Windows 11', time: 'Yesterday, 18:42' },
  { id: 'a3', action: 'Signed in', detail: 'Safari · iPhone 15', time: '16 Sep, 08:31' },
  { id: 'a4', action: 'Signed out', detail: 'Safari · iPhone 15', time: '15 Sep, 17:05' },
]

/**
 * Change-password form with frontend validation only.
 */
function ChangePasswordForm({ onSaved }) {
  const { toast } = useToast()
  const [form, setForm] = useState({ current: '', next: '', confirm: '' })
  const [errors, setErrors] = useState({})

  function save() {
    const errs = {}
    if (!form.current) errs.current = 'Enter your current password.'
    if (!form.next) errs.next = 'Enter a new password.'
    else if (form.next.length < 6) errs.next = 'New password must be at least 6 characters.'
    else if (form.next === form.current) errs.next = 'Use a different password from the current one.'
    if (!form.confirm) errs.confirm = 'Confirm the new password.'
    else if (form.confirm !== form.next) errs.confirm = 'Passwords do not match.'
    setErrors(errs)
    if (Object.keys(errs).length) return

    setForm({ current: '', next: '', confirm: '' })
    onSaved?.()
    toast('Password updated successfully.', 'success')
  }

  return (
    <div>
      <div className="font-semibold" style={{ marginBottom: 6 }}>Change password</div>
      <p className="text-xs text-muted" style={{ marginBottom: 14 }}>
        Password changes are frontend-simulated until the backend is connected.
      </p>
      <div className="form-row">
        <Field label="Current password" required error={errors.current} htmlFor="cp-current">
          <Input
            id="cp-current"
            type="password"
            autoComplete="current-password"
            value={form.current}
            error={errors.current}
            onChange={(e) => setForm({ ...form, current: e.target.value })}
          />
        </Field>
        <Field label="New password" required error={errors.next} htmlFor="cp-next">
          <Input
            id="cp-next"
            type="password"
            autoComplete="new-password"
            value={form.next}
            error={errors.next}
            onChange={(e) => setForm({ ...form, next: e.target.value })}
          />
        </Field>
      </div>
      <Field label="Confirm new password" required error={errors.confirm} htmlFor="cp-confirm">
        <Input
          id="cp-confirm"
          type="password"
          autoComplete="new-password"
          value={form.confirm}
          error={errors.confirm}
          onChange={(e) => setForm({ ...form, confirm: e.target.value })}
        />
      </Field>
      <div className="settings-save-row">
        <button className="btn btn-primary" type="button" onClick={save}>
          <KeyRound size={15} /> Update Password
        </button>
      </div>
    </div>
  )
}

/**
 * Security: change password + active sessions + login activity + sign out
 * everything. Session/logout flows are simulated placeholders.
 */
export default function SecuritySection() {
  const { toast } = useToast()
  const [sessions, setSessions] = useState(MOCK_SESSIONS)
  const [confirmLogoutAll, setConfirmLogoutAll] = useState(false)

  function endSession(id) {
    setSessions((list) => list.filter((s) => s.id !== id))
    toast('Session ended.', 'success')
  }

  return (
    <>
      <SectionHead
        icon={ShieldCheck}
        tone="blue"
        title="Security"
        desc="Keep your account safe - passwords, sessions and sign-in activity."
      />

      <ChangePasswordForm />

      <hr className="divider" />

      <div className="row" style={{ marginBottom: 10 }}>
        <Monitor size={16} className="text-muted" />
        <div className="font-semibold" style={{ fontSize: 14.5 }}>Active sessions</div>
      </div>
      {sessions.length === 0 ? (
        <p className="text-sm text-secondary">No other active sessions.</p>
      ) : (
        sessions.map((s) => {
          const Icon = s.icon
          return (
            <div className="session-row" key={s.id}>
              <div className="session-icon"><Icon size={16} /></div>
              <div style={{ minWidth: 0, flex: 1 }}>
                <div className="session-device">
                  <span className="font-semibold" style={{ fontSize: 13.5 }}>{s.device}</span>
                  {s.current && <span className="badge badge-primary" style={{ marginLeft: 8 }}>This device</span>}
                </div>
                <div className="text-xs text-muted">{s.location} · {s.lastActive}</div>
              </div>
              {!s.current && (
                <button className="btn btn-ghost btn-sm" onClick={() => endSession(s.id)}>
                  Log out
                </button>
              )}
            </div>
          )
        })
      )}

      <hr className="divider" />

      <div className="row" style={{ marginBottom: 10 }}>
        <Clock size={16} className="text-muted" />
        <div className="font-semibold" style={{ fontSize: 14.5 }}>Login activity</div>
      </div>
      <div className="activity-list">
        {MOCK_ACTIVITY.map((a) => (
          <div className="activity-row" key={a.id}>
            <div className="activity-dot" />
            <div style={{ flex: 1, minWidth: 0 }}>
              <div className="font-medium text-sm" style={{ fontSize: 13.5 }}>{a.action}</div>
              <div className="text-xs text-muted">{a.detail} · {a.time}</div>
            </div>
          </div>
        ))}
      </div>

      <hr className="divider" />

      <div className="row-between" style={{ gap: 14 }}>
        <div>
          <div className="font-semibold">Log out of all devices</div>
          <div className="text-xs text-muted" style={{ marginTop: 2 }}>
            Ends every other session; this device stays signed in.
          </div>
        </div>
        <button className="btn btn-danger" onClick={() => setConfirmLogoutAll(true)}>
          <LogOut size={15} /> Log out all
        </button>
      </div>

      <ConfirmDialog
        open={confirmLogoutAll}
        title="Log out of all other devices?"
        confirmLabel="Log out all"
        message="All active sessions on other devices will be ended. You will stay signed in on this device."
        onClose={() => setConfirmLogoutAll(false)}
        onConfirm={() => {
          setConfirmLogoutAll(false)
          toast('Signed out of all other devices.', 'success')
        }}
      />
    </>
  )
}