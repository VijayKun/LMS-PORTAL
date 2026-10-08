import { LifeBuoy, HelpCircle, PhoneCall, Flag } from 'lucide-react'
import { useToast } from '../../store/ToastContext.jsx'
import SectionHead from './SectionHead.jsx'

const HELP_ITEMS = [
  {
    key: 'help',
    icon: LifeBuoy,
    title: 'Help Center',
    desc: 'Guides on borrowing, returns, reservations and fines.',
  },
  {
    key: 'faq',
    icon: HelpCircle,
    title: 'Frequently Asked Questions',
    desc: 'Quick answers for the most common library questions.',
  },
  {
    key: 'contact',
    icon: PhoneCall,
    title: 'Contact Library',
    desc: 'Reach the front desk, email or phone of the library.',
  },
  {
    key: 'report',
    icon: Flag,
    title: 'Report a Problem',
    desc: 'Tell us about a broken page or an issue with the portal.',
  },
]

/**
 * Help & Support placeholders - each card is an action that will open a real
 * help flow once the backend and knowledge base pages are connected.
 */
export default function HelpSupportSection() {
  const { toast } = useToast()

  return (
    <>
      <SectionHead
        icon={LifeBuoy}
        tone="blue"
        title="Help & Support"
        desc="Find answers or get in touch with the library team."
      />
      <div className="help-grid">
        {HELP_ITEMS.map((item) => {
          const Icon = item.icon
          return (
            <button
              type="button"
              key={item.key}
              className="help-card"
              onClick={() => toast(`${item.title} will be available in the full release.`, 'info')}
            >
              <div className="stat-icon cyan" style={{ flexShrink: 0 }}>
                <Icon size={18} />
              </div>
              <span>
                <span className="font-semibold" style={{ fontSize: 13.5 }}>{item.title}</span>
                <span style={{ display: 'block', fontSize: 12.6, color: 'var(--text-secondary)', marginTop: 2 }}>
                  {item.desc}
                </span>
              </span>
            </button>
          )
        })}
      </div>
    </>
  )
}