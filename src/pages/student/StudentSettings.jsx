import { useState } from 'react'
import {
  Bell,
  MonitorSmartphone,
  ShieldCheck,
  Languages,
  Lock,
  LifeBuoy,
  Info,
  CheckCheck,
  Ban,
} from 'lucide-react'
import { useLibrary } from '../../store/LibraryContext.jsx'
import { useAuth } from '../../store/AuthContext.jsx'
import { usePageTitle } from '../../store/PageTitleContext.jsx'
import { useToast } from '../../store/ToastContext.jsx'
import Switch from '../../components/ui/Switch.jsx'
import SettingsNav from '../../components/settings/SettingsNav.jsx'
import SectionHead from '../../components/settings/SectionHead.jsx'
import AppearanceSection from '../../components/settings/AppearanceSection.jsx'
import SecuritySection from '../../components/settings/SecuritySection.jsx'
import LanguageSection from '../../components/settings/LanguageSection.jsx'
import PrivacySection from '../../components/settings/PrivacySection.jsx'
import HelpSupportSection from '../../components/settings/HelpSupportSection.jsx'
import AboutSection from '../../components/settings/AboutSection.jsx'
import { defaultSettings, STUDENT_NOTIFICATIONS } from '../../data/settings.js'

const merge = (obj, fallback) => ({ ...fallback, ...(obj || {}) })

const DEFAULT_CHANNELS = {
  inApp: Object.fromEntries(STUDENT_NOTIFICATIONS.map((t) => [t.key, true])),
  email: Object.fromEntries(STUDENT_NOTIFICATIONS.map((t) => [t.key, true])),
}

/* ------------------------------------------------------------------ */
/*  Notifications: per-channel toggles (in-app + email).               */
/* ------------------------------------------------------------------ */
function StudentNotifications({ prefs, onChange }) {
  const { toast } = useToast()

  function toggle(key, channel, value) {
    onChange({ ...prefs, [channel]: { ...prefs[channel], [key]: value } })
  }

  function setAll(value) {
    onChange({ inApp: Object.fromEntries(STUDENT_NOTIFICATIONS.map((t) => [t.key, value])), email: Object.fromEntries(STUDENT_NOTIFICATIONS.map((t) => [t.key, value])) })
    toast(value ? 'All notifications enabled.' : 'All notifications disabled.', 'success')
  }

  const allOn =
    STUDENT_NOTIFICATIONS.every((t) => prefs.inApp[t.key]) &&
    STUDENT_NOTIFICATIONS.every((t) => prefs.email[t.key])
  const allOff =
    STUDENT_NOTIFICATIONS.every((t) => !prefs.inApp[t.key]) &&
    STUDENT_NOTIFICATIONS.every((t) => !prefs.email[t.key])

  return (
    <>
      <SectionHead
        icon={Bell}
        tone="amber"
        title="Notifications"
        desc="Choose what you want to be reminded about, in-app and by email."
      />
      <div className="row" style={{ gap: 10, marginBottom: 10 }}>
        <button className={`btn btn-sm ${allOn ? 'btn-primary' : 'btn-secondary'}`} onClick={() => setAll(true)}>
          <CheckCheck size={15} /> Enable all
        </button>
        <button className={`btn btn-sm ${allOff ? 'btn-primary' : 'btn-secondary'}`} onClick={() => setAll(false)}>
          <Ban size={15} /> Disable all
        </button>
      </div>

      <div className="pref-grid head">
        <span>Preference</span>
        <span className="pref-chan">In-app</span>
        <span className="pref-chan">Email</span>
      </div>
      {STUDENT_NOTIFICATIONS.map((t) => (
        <div className="pref-grid" key={t.key}>
          <div>
            <div className="pref-name">{t.label}</div>
            <div className="pref-hint">{t.hint}</div>
          </div>
          <div className="pref-chan">
            <Switch
              checked={prefs.inApp[t.key]}
              onChange={(v) => toggle(t.key, 'inApp', v)}
              label={`${t.label} - in-app`}
            />
          </div>
          <div className="pref-chan">
            <Switch
              checked={prefs.email[t.key]}
              onChange={(v) => toggle(t.key, 'email', v)}
              label={`${t.label} - email`}
            />
          </div>
        </div>
      ))}
    </>
  )
}

/* ------------------------------------------------------------------ */
/*  Page shell.                                                        */
/* ------------------------------------------------------------------ */
export default function StudentSettings() {
  usePageTitle('Settings')
  const { state, updateSettings } = useLibrary()
  const { user } = useAuth()
  const { toast } = useToast()

  const settings = merge(state.settings, defaultSettings)
  const uid = user?.id
  const studentPref = merge(settings.studentPrefs?.[uid], {
    language: 'en',
    privacy: { ...defaultSettings.privacy },
  })

  const [active, setActive] = useState('notifications')

  const SECTIONS = [
    { id: 'notifications', label: 'Notifications', icon: Bell },
    { id: 'appearance', label: 'Appearance', icon: MonitorSmartphone },
    { id: 'security', label: 'Security', icon: ShieldCheck },
    { id: 'language', label: 'Language', icon: Languages },
    { id: 'privacy', label: 'Privacy', icon: Lock },
    { id: 'help', label: 'Help & Support', icon: LifeBuoy },
    { id: 'about', label: 'About', icon: Info },
  ]

  function saveStudentPrefs(patch, message = 'Preferences saved.') {
    updateSettings({
      studentPrefs: { ...(settings.studentPrefs || {}), [uid]: { ...studentPref, ...patch } },
    })
    if (message) toast(message, 'success')
  }

  return (
    <>
      <div className="page-header">
        <div>
          <h1>Settings</h1>
          <p className="page-sub">Manage your account and preferences.</p>
        </div>
      </div>

      <div className="settings-layout">
        <SettingsNav items={SECTIONS} active={active} onChange={setActive} />
        <section className="card settings-panel">
          {active === 'notifications' && (
            <StudentNotifications
              prefs={merge(settings.studentNotifications?.[uid], DEFAULT_CHANNELS)}
              onChange={(next) =>
                updateSettings({
                  studentNotifications: { ...(settings.studentNotifications || {}), [uid]: next },
                })
              }
            />
          )}
          {active === 'appearance' && <AppearanceSection />}
          {active === 'security' && <SecuritySection />}
          {active === 'language' && (
            <LanguageSection
              value={studentPref.language}
              onChange={(v) => saveStudentPrefs({ language: v }, 'Language preference saved.')}
            />
          )}
          {active === 'privacy' && (
            <PrivacySection
              privacy={merge(studentPref.privacy, defaultSettings.privacy)}
              onChange={(p) => saveStudentPrefs({ privacy: p })}
            />
          )}
          {active === 'help' && <HelpSupportSection />}
          {active === 'about' && <AboutSection />}
        </section>
      </div>
    </>
  )
}