import { Info, FileText, ShieldCheck, LibraryBig } from 'lucide-react'
import { useToast } from '../../store/ToastContext.jsx'
import { APP_VERSION } from '../../data/settings.js'
import SectionHead from './SectionHead.jsx'

const POLICIES = [
  {
    title: 'Terms & Conditions',
    desc: 'Rules for using the library and the portal.',
    icon: FileText,
  },
  {
    title: 'Privacy Policy',
    desc: 'How we collect, use and protect your data.',
    icon: ShieldCheck,
  },
]

/**
 * About the Library Management System. Static, project-facing information.
 */
export default function AboutSection() {
  const { toast } = useToast()

  return (
    <>
      <SectionHead
        icon={Info}
        tone="cyan"
        title="About"
        desc="About the Library Management System."
      />
      <div className="about-hero" style={{ display: 'flex', alignItems: 'center', gap: 16 }}>
        <div className="about-logo">
          <LibraryBig size={30} />
        </div>
        <div>
          <h3 style={{ fontSize: 17 }}>Library Management System</h3>
          <div className="text-sm text-muted">Version {APP_VERSION} (demo build)</div>
        </div>
      </div>

      <p className="text-secondary" style={{ marginTop: 16, fontSize: 14 }}>
        A modern portal for a college library - catalog, borrowing, reservations,
        fines and reporting. This frontend runs on mock data today and is designed
        to connect to a C HTTP backend with PostgreSQL for production.
      </p>

      <hr className="divider" />

      {POLICIES.map((p) => {
        const Icon = p.icon
        return (
          <button
            type="button"
            key={p.title}
            className="help-card"
            style={{ width: '100%', marginBottom: 10 }}
            onClick={() => toast(`${p.title} will be linked in the full release.`, 'info')}
          >
            <div className="stat-icon blue" style={{ flexShrink: 0 }}>
              <Icon size={18} />
            </div>
            <span>
              <span className="font-semibold" style={{ fontSize: 13.5 }}>{p.title}</span>
              <span style={{ display: 'block', fontSize: 12.6, color: 'var(--text-secondary)', marginTop: 2 }}>
                {p.desc}
              </span>
            </span>
          </button>
        )
      })}
    </>
  )
}