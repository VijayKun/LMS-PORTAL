import { useState } from 'react'
import {
  UserRound,
  Bell,
  MonitorSmartphone,
  ShieldCheck,
  Languages,
  Lock,
  Library,
  ArrowLeftRight,
  Mail,
  LifeBuoy,
  Info,
  Pencil,
  Save,
  Camera,
  Trash2,
  X,
  CheckCheck,
  Ban,
  Send,
  RotateCcw,
} from 'lucide-react'
import { useLibrary } from '../../store/LibraryContext.jsx'
import { useAuth } from '../../store/AuthContext.jsx'
import { usePageTitle } from '../../store/PageTitleContext.jsx'
import { useToast } from '../../store/ToastContext.jsx'
import { Field, Input } from '../../components/ui/Field.jsx'
import ConfirmDialog from '../../components/ui/ConfirmDialog.jsx'
import SettingsNav from '../../components/settings/SettingsNav.jsx'
import SectionHead from '../../components/settings/SectionHead.jsx'
import ToggleRow from '../../components/settings/ToggleRow.jsx'
import AppearanceSection from '../../components/settings/AppearanceSection.jsx'
import SecuritySection from '../../components/settings/SecuritySection.jsx'
import LanguageSection from '../../components/settings/LanguageSection.jsx'
import PrivacySection from '../../components/settings/PrivacySection.jsx'
import HelpSupportSection from '../../components/settings/HelpSupportSection.jsx'
import AboutSection from '../../components/settings/AboutSection.jsx'
import { defaultSettings, ADMIN_NOTIFICATIONS, ALL_DAYS } from '../../data/settings.js'
import { getInitials } from '../../utils/format.js'

const merge = (obj, fallback) => ({ ...fallback, ...(obj || {}) })

/* ------------------------------------------------------------------ */
/*  Profile: picture, name, email, phone. Role is never editable.      */
/* ------------------------------------------------------------------ */
function AdminProfile({ user, updateUser }) {
  const { toast } = useToast()
  const [editing, setEditing] = useState(false)
  const [form, setForm] = useState({
    name: user?.name || '',
    email: user?.email || '',
    contact: user?.contact || '',
  })
  const [errors, setErrors] = useState({})

  function openEdit() {
    setForm({ name: user?.name || '', email: user?.email || '', contact: user?.contact || '' })
    setErrors({})
    setEditing(true)
  }

  function cancel() {
    setEditing(false)
    setErrors({})
  }

  function save() {
    const errs = {}
    if (!form.name.trim()) errs.name = 'Full name is required.'
    if (!/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(form.email)) errs.email = 'Enter a valid email.'
    if (!/^[+\d][\d\s-]{7,}$/.test(form.contact)) errs.contact = 'Enter a valid phone number.'
    setErrors(errs)
    if (Object.keys(errs).length) return
    updateUser({ name: form.name.trim(), email: form.email.trim(), contact: form.contact.trim() })
    setEditing(false)
    toast('Profile saved successfully.', 'success')
  }

  function onPhotoFile(e) {
    const file = e.target.files?.[0]
    e.target.value = ''
    if (!file) return
    if (!file.type.startsWith('image/')) {
      toast('Please choose an image file.', 'error')
      return
    }
    if (file.size > 400 * 1024) {
      toast('The image is larger than 400 KB.', 'error')
      return
    }
    const reader = new FileReader()
    reader.onload = () => {
      updateUser({ avatar: reader.result })
      toast('Profile picture updated.', 'success')
    }
    reader.readAsDataURL(file)
  }

  return (
    <>
      <SectionHead
        icon={UserRound}
        tone="blue"
        title="Profile"
        desc="Your account details as shown to library staff."
      />

      <div className="row" style={{ gap: 16, marginBottom: 22 }}>
        {user?.avatar ? (
          <img className="avatar avatar-lg avatar-img" src={user.avatar} alt="Profile picture" />
        ) : (
          <div className="avatar avatar-lg">{getInitials(user?.name)}</div>
        )}
        <div className="row" style={{ gap: 8, flexWrap: 'wrap' }}>
          <label className="btn btn-secondary btn-sm avatar-btn">
            <Camera size={14} /> Change photo
            <input type="file" accept="image/*" hidden onChange={onPhotoFile} />
          </label>
          {user?.avatar && (
            <button
              className="btn btn-ghost btn-sm"
              onClick={() => {
                updateUser({ avatar: null })
                toast('Profile picture removed.', 'success')
              }}
            >
              <Trash2 size={14} /> Remove
            </button>
          )}
        </div>
      </div>

      <Field label="Full name" required error={errors.name} htmlFor="adp-name">
        <Input
          id="adp-name"
          value={form.name}
          disabled={!editing}
          error={errors.name}
          onChange={(e) => setForm({ ...form, name: e.target.value })}
        />
      </Field>
      <div className="form-row">
        <Field label="Email" required error={errors.email} htmlFor="adp-email">
          <Input
            id="adp-email"
            type="email"
            value={form.email}
            disabled={!editing}
            error={errors.email}
            onChange={(e) => setForm({ ...form, email: e.target.value })}
          />
        </Field>
        <Field label="Phone number" required error={errors.contact} htmlFor="adp-phone">
          <Input
            id="adp-phone"
            value={form.contact}
            disabled={!editing}
            error={errors.contact}
            onChange={(e) => setForm({ ...form, contact: e.target.value })}
          />
        </Field>
      </div>
      <Field label="Role" hint="Your role (Administrator) is assigned by the library and cannot be changed.">
        <Input value={user?.title || 'Administrator'} disabled />
      </Field>

      <div className="settings-save-row">
        {editing ? (
          <>
            <button className="btn btn-ghost" onClick={cancel}>
              <X size={15} /> Cancel
            </button>
            <button className="btn btn-primary" onClick={save}>
              <Save size={15} /> Save Changes
            </button>
          </>
        ) : (
          <button className="btn btn-primary" onClick={openEdit}>
            <Pencil size={15} /> Edit Profile
          </button>
        )}
      </div>
    </>
  )
}

/* ------------------------------------------------------------------ */
/*  Notifications: admin toggles with enable/disable all.              */
/* ------------------------------------------------------------------ */
function AdminNotifications({ notifications, onSave }) {
  const { toast } = useToast()
  const allOn = ADMIN_NOTIFICATIONS.every((t) => notifications[t.key])
  const allOff = ADMIN_NOTIFICATIONS.every((t) => !notifications[t.key])

  function set(key, value) {
    onSave({ notifications: { ...notifications, [key]: value } }, 'Notification preferences updated.')
  }

  function setAll(value) {
    const next = Object.fromEntries(ADMIN_NOTIFICATIONS.map((t) => [t.key, value]))
    onSave({ notifications: { ...notifications, ...next } })
    toast(value ? 'All notifications enabled.' : 'All notifications disabled.', 'success')
  }

  return (
    <>
      <SectionHead
        icon={Bell}
        tone="amber"
        title="Notifications"
        desc="Choose the library alerts you receive while using the portal."
      />
      <div className="row" style={{ gap: 10, marginBottom: 10 }}>
        <button className={`btn btn-sm ${allOn ? 'btn-primary' : 'btn-secondary'}`} onClick={() => setAll(true)}>
          <CheckCheck size={15} /> Enable all
        </button>
        <button className={`btn btn-sm ${allOff ? 'btn-primary' : 'btn-secondary'}`} onClick={() => setAll(false)}>
          <Ban size={15} /> Disable all
        </button>
      </div>
      {ADMIN_NOTIFICATIONS.map((t) => (
        <ToggleRow
          key={t.key}
          label={t.label}
          hint={t.hint}
          checked={notifications[t.key]}
          onChange={(v) => set(t.key, v)}
        />
      ))}
    </>
  )
}

/* ------------------------------------------------------------------ */
/*  Library Settings: name, contact and opening hours.                 */
/* ------------------------------------------------------------------ */
function LibrarySettings({ library, onSave }) {
  const { toast } = useToast()
  const [form, setForm] = useState(library)
  const [errors, setErrors] = useState({})

  function toggleDay(day) {
    setForm({
      ...form,
      workingDays: form.workingDays.includes(day)
        ? form.workingDays.filter((d) => d !== day)
        : [...form.workingDays, day],
    })
  }

  function save() {
    const errs = {}
    if (!form.name.trim()) errs.name = 'Library name is required.'
    if (!/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(form.email)) errs.email = 'Enter a valid email.'
    if (!/^[+\d][\d\s-]{7,}$/.test(form.phone)) errs.phone = 'Enter a valid phone number.'
    if (!form.openingTime || !form.closingTime) errs.hours = 'Set both opening and closing time.'
    setErrors(errs)
    if (Object.keys(errs).length) return
    onSave({ library: { ...form } }, 'Library settings saved successfully.')
    toast('Library settings saved.', 'success')
  }

  return (
    <>
      <SectionHead
        icon={Library}
        tone="cyan"
        title="Library Settings"
        desc="Basic identity and working hours shown across the portal."
      />
      <Field label="Library name" required error={errors.name} htmlFor="lib-name">
        <Input
          id="lib-name"
          value={form.name}
          error={errors.name}
          onChange={(e) => setForm({ ...form, name: e.target.value })}
        />
      </Field>
      <div className="form-row">
        <Field label="Library email" required error={errors.email} htmlFor="lib-email">
          <Input
            id="lib-email"
            type="email"
            value={form.email}
            error={errors.email}
            onChange={(e) => setForm({ ...form, email: e.target.value })}
          />
        </Field>
        <Field label="Library phone" required error={errors.phone} htmlFor="lib-phone">
          <Input
            id="lib-phone"
            value={form.phone}
            error={errors.phone}
            onChange={(e) => setForm({ ...form, phone: e.target.value })}
          />
        </Field>
      </div>
      <Field label="Library address" htmlFor="lib-address">
        <Input
          id="lib-address"
          value={form.address}
          onChange={(e) => setForm({ ...form, address: e.target.value })}
        />
      </Field>
      <div className="form-row">
        <Field label="Opening time" required error={errors.hours} htmlFor="lib-open">
          <Input
            id="lib-open"
            type="time"
            value={form.openingTime}
            error={errors.hours}
            onChange={(e) => setForm({ ...form, openingTime: e.target.value })}
          />
        </Field>
        <Field label="Closing time" required htmlFor="lib-close">
          <Input
            id="lib-close"
            type="time"
            value={form.closingTime}
            onChange={(e) => setForm({ ...form, closingTime: e.target.value })}
          />
        </Field>
      </div>
      <Field label="Working days">
        <div className="day-chips">
          {ALL_DAYS.map((day) => (
            <button
              type="button"
              key={day}
              className={`day-chip${form.workingDays.includes(day) ? ' on' : ''}`}
              onClick={() => toggleDay(day)}
            >
              {day}
            </button>
          ))}
        </div>
      </Field>
      <div className="settings-save-row">
        <button className="btn btn-primary" onClick={save}>
          <Save size={15} /> Save Changes
        </button>
      </div>
    </>
  )
}

/* ------------------------------------------------------------------ */
/*  Circulation Settings: library rules.                               */
/* ------------------------------------------------------------------ */
function CirculationSettings({ circulation, onSave }) {
  const { toast } = useToast()
  const [form, setForm] = useState(circulation)
  const [errors, setErrors] = useState({})

  const NUMBER_FIELDS = [
    { key: 'maxBooksPerStudent', label: 'Maximum books per student', min: 1, max: 20 },
    { key: 'borrowingPeriodDays', label: 'Borrowing period (days)', min: 1, max: 90 },
    { key: 'finePerDay', label: 'Fine per overdue day (₹)', min: 0, max: 1000 },
    { key: 'maximumFine', label: 'Maximum fine (₹)', min: 0, max: 5000 },
    { key: 'maxRenewalsPerBook', label: 'Maximum renewals', min: 0, max: 10 },
    { key: 'reservationValidityDays', label: 'Reservation validity (days)', min: 1, max: 30 },
    { key: 'maxActiveReservations', label: 'Maximum active reservations', min: 1, max: 20 },
  ]

  function save() {
    const errs = {}
    for (const f of NUMBER_FIELDS) {
      const value = Number(form[f.key])
      if (!Number.isFinite(value) || value < f.min || value > f.max) {
        errs[f.key] = `Between ${f.min} and ${f.max}.`
      }
    }
    setErrors(errs)
    if (Object.keys(errs).length) return
    const next = {}
    for (const f of NUMBER_FIELDS) next[f.key] = Number(form[f.key])
    onSave({ circulation: { ...form, ...next } }, 'Circulation rules saved successfully.')
    toast('Circulation rules saved.', 'success')
  }

  function setNum(key, value) {
    setForm({ ...form, [key]: value })
  }

  return (
    <>
      <SectionHead
        icon={ArrowLeftRight}
        tone="blue"
        title="Circulation Settings"
        desc="Library rules for borrowing, renewals, reservations and fines."
      />

      <div className="form-row" style={{ marginBottom: 2 }}>
        <Field label="Maximum books per student" error={errors.maxBooksPerStudent} htmlFor="cir-max-books">
          <Input
            id="cir-max-books"
            type="number"
            min="1"
            max="20"
            value={form.maxBooksPerStudent}
            error={errors.maxBooksPerStudent}
            onChange={(e) => setNum('maxBooksPerStudent', e.target.value)}
          />
        </Field>
        <Field label="Borrowing period (days)" error={errors.borrowingPeriodDays} htmlFor="cir-period">
          <Input
            id="cir-period"
            type="number"
            min="1"
            max="90"
            value={form.borrowingPeriodDays}
            error={errors.borrowingPeriodDays}
            onChange={(e) => setNum('borrowingPeriodDays', e.target.value)}
          />
        </Field>
      </div>
      <div className="form-row">
        <Field label="Fine per overdue day (₹)" error={errors.finePerDay} htmlFor="cir-fine">
          <Input
            id="cir-fine"
            type="number"
            min="0"
            max="1000"
            value={form.finePerDay}
            error={errors.finePerDay}
            onChange={(e) => setNum('finePerDay', e.target.value)}
          />
        </Field>
        <Field label="Maximum fine (₹)" error={errors.maximumFine} htmlFor="cir-max-fine">
          <Input
            id="cir-max-fine"
            type="number"
            min="0"
            max="5000"
            value={form.maximumFine}
            error={errors.maximumFine}
            onChange={(e) => setNum('maximumFine', e.target.value)}
          />
        </Field>
      </div>

      <ToggleRow
        label="Allow book renewal"
        hint="Let students extend the borrowing period on issued books."
        checked={form.renewalEnabled}
        onChange={(v) => setForm({ ...form, renewalEnabled: v })}
      />
      <Field label="Maximum renewals" disabled={!form.renewalEnabled} hint={form.renewalEnabled ? undefined : 'Turn on renewal to set this.'} htmlFor="cir-renewals">
        <Input
          id="cir-renewals"
          type="number"
          min="0"
          max="10"
          value={form.maxRenewalsPerBook}
          disabled={!form.renewalEnabled}
          error={errors.maxRenewalsPerBook}
          onChange={(e) => setNum('maxRenewalsPerBook', e.target.value)}
        />
      </Field>

      <div className="form-row">
        <Field label="Reservation validity (days)" error={errors.reservationValidityDays} htmlFor="cir-res-valid">
          <Input
            id="cir-res-valid"
            type="number"
            min="1"
            max="30"
            value={form.reservationValidityDays}
            error={errors.reservationValidityDays}
            onChange={(e) => setNum('reservationValidityDays', e.target.value)}
          />
        </Field>
        <Field label="Maximum active reservations" error={errors.maxActiveReservations} htmlFor="cir-max-res">
          <Input
            id="cir-max-res"
            type="number"
            min="1"
            max="20"
            value={form.maxActiveReservations}
            error={errors.maxActiveReservations}
            onChange={(e) => setNum('maxActiveReservations', e.target.value)}
          />
        </Field>
      </div>

      <div className="settings-save-row">
        <button className="btn btn-primary" onClick={save}>
          <Save size={15} /> Save Changes
        </button>
      </div>

      <div className="settings-note" style={{ marginTop: 16 }}>
        <Info size={17} />
        <span>
          These rules are stored locally for now. When the C backend is connected they
          map to the library database (maximum_books, borrowing_period_days, fine_per_day,
          maximum_fine, renewal_enabled, maximum_renewals, reservation_validity_days,
          maximum_reservations).
        </span>
      </div>
    </>
  )
}

/* ------------------------------------------------------------------ */
/*  Email Settings: master switch + per-rule toggles + test button.    */
/* ------------------------------------------------------------------ */
function EmailSettings({ email, onSave }) {
  const { toast } = useToast()
  const [sending, setSending] = useState(false)

  const OPTIONS = [
    { key: 'dueDate', label: 'Due-date reminder emails' },
    { key: 'overdue', label: 'Overdue emails' },
    { key: 'fine', label: 'Fine emails' },
    { key: 'reservation', label: 'Reservation emails' },
    { key: 'announcement', label: 'Library announcement emails' },
  ]

  function set(key, value) {
    onSave({ email: { ...email, [key]: value } })
  }

  function sendTest() {
    setSending(true)
    setTimeout(() => {
      setSending(false)
      toast(
        'Test email queued. The email service will be connected through the backend.',
        'info',
      )
    }, 900)
  }

  return (
    <>
      <SectionHead
        icon={Mail}
        tone="green"
        title="Email Settings"
        desc="Outbound email for reminders, fines and announcements."
      />
      <ToggleRow
        label="Enable email notifications"
        hint="Master switch for all outbound email from the library."
        checked={email.enabled}
        onChange={(v) => set('enabled', v)}
      />
      {OPTIONS.map((o) => (
        <ToggleRow
          key={o.key}
          label={o.label}
          checked={email[o.key]}
          disabled={!email.enabled}
          onChange={(v) => set(o.key, v)}
        />
      ))}
      <div className="settings-save-row">
        <button className="btn btn-secondary" onClick={sendTest} disabled={sending || !email.enabled}>
          {!sending && <Send size={15} />}
          {sending ? 'Sending…' : 'Send test email'}
        </button>
      </div>
      <div className="settings-note" style={{ marginTop: 16 }}>
        <Mail size={17} />
        <span>
          The email service is configured through the backend - no API keys or mail
          credentials are stored in the frontend.
        </span>
      </div>
    </>
  )
}

/* ------------------------------------------------------------------ */
/*  Page shell: defines the section list and renders the active one.   */
/* ------------------------------------------------------------------ */
export default function Settings() {
  usePageTitle('Settings')
  const { state, updateSettings, resetData } = useLibrary()
  const { user, updateUser } = useAuth()
  const { toast } = useToast()

  const settings = merge(state.settings, defaultSettings)
  const [active, setActive] = useState('profile')
  const [confirmReset, setConfirmReset] = useState(false)

  const SECTIONS = [
    { id: 'profile', label: 'Profile', icon: UserRound },
    { id: 'notifications', label: 'Notifications', icon: Bell },
    { id: 'appearance', label: 'Appearance', icon: MonitorSmartphone },
    { id: 'security', label: 'Security', icon: ShieldCheck },
    { id: 'language', label: 'Language', icon: Languages },
    { id: 'privacy', label: 'Privacy', icon: Lock },
    { id: 'library', label: 'Library Settings', icon: Library },
    { id: 'circulation', label: 'Circulation', icon: ArrowLeftRight },
    { id: 'email', label: 'Email', icon: Mail },
    { id: 'help', label: 'Help & Support', icon: LifeBuoy },
    { id: 'about', label: 'About', icon: Info },
  ]

  function saveSettings(patch, message = 'Settings saved successfully.') {
    updateSettings(patch)
    toast(message, 'success')
  }

  function doReset() {
    resetData()
    setConfirmReset(false)
    toast('Demo data restored to its original state.', 'success')
  }

  return (
    <>
      <div className="page-header">
        <div>
          <h1>Settings</h1>
          <p className="page-sub">Manage your account and library configuration.</p>
        </div>
      </div>

      <div className="settings-layout">
        <SettingsNav items={SECTIONS} active={active} onChange={setActive} />
        <section className="card settings-panel">
          {active === 'profile' && <AdminProfile user={user} updateUser={updateUser} />}
          {active === 'notifications' && (
            <AdminNotifications notifications={merge(settings.notifications, defaultSettings.notifications)} onSave={saveSettings} />
          )}
          {active === 'appearance' && <AppearanceSection />}
          {active === 'security' && <SecuritySection />}
          {active === 'language' && (
            <LanguageSection
              value={settings.language}
              onChange={(v) => saveSettings({ language: v }, 'Language preference saved.')}
            />
          )}
          {active === 'privacy' && (
            <PrivacySection
              privacy={merge(settings.privacy, defaultSettings.privacy)}
              onChange={(p) => saveSettings({ privacy: p })}
            />
          )}
          {active === 'library' && (
            <LibrarySettings library={merge(settings.library, defaultSettings.library)} onSave={saveSettings} />
          )}
          {active === 'circulation' && (
            <CirculationSettings circulation={merge(settings.circulation, defaultSettings.circulation)} onSave={saveSettings} />
          )}
          {active === 'email' && (
            <EmailSettings email={merge(settings.email, defaultSettings.email)} onSave={saveSettings} />
          )}
          {active === 'help' && <HelpSupportSection />}
          {active === 'about' && (
            <>
              <AboutSection />
              <hr className="divider" />
              <div className="row-between" style={{ gap: 14 }}>
                <div>
                  <div className="font-semibold">Reset demo data</div>
                  <div className="text-xs text-muted" style={{ marginTop: 2 }}>
                    Restores the original mock books, students, transactions and settings.
                  </div>
                </div>
                <button className="btn btn-danger" onClick={() => setConfirmReset(true)}>
                  <RotateCcw size={15} /> Reset data
                </button>
              </div>
            </>
          )}
        </section>
      </div>

      <ConfirmDialog
        open={confirmReset}
        title="Reset all demo data?"
        confirmLabel="Reset Data"
        message="Every change you made in this browser (books, students, borrows, fines, settings) will be replaced by the original mock dataset."
        onClose={() => setConfirmReset(false)}
        onConfirm={doReset}
      />
    </>
  )
}