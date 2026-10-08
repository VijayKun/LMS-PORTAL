import { NavLink } from 'react-router-dom'
import { X, PanelLeftClose, PanelLeftOpen } from 'lucide-react'
import BrandLogo from '../BrandLogo.jsx'

/**
 * Left navigation rail (navy design).
 * Props:
 *   sections  - [{ label, links: [{ to, label, icon, end, badge }] }]
 *   open      - mobile drawer visibility
 *   onClose   - closes the drawer after a link is tapped
 *   collapsed - desktop icon-only rail
 *   onToggleCollapse - switches between rail and full sidebar
 */
export default function Sidebar({ sections, open, onClose, brand, brandSub, collapsed, onToggleCollapse }) {
  return (
    <>
      {open && <div className="mobile-overlay" onClick={onClose} />}
      <aside className={`sidebar ${open ? 'open' : ''}`}>
        <div className="sidebar-brand">
          <div className="sidebar-logo">
            <BrandLogo />
          </div>
          <div className="flex-1">
            <div className="sidebar-brand-text">{brand}</div>
            <div className="sidebar-brand-sub">{brandSub}</div>
          </div>
          <button className="icon-btn" onClick={onClose} aria-label="Close menu" id="sidebar-close-btn">
            <X size={17} className="sidebar-close-icon" />
          </button>
        </div>

        <button
          className="sidebar-toggle"
          onClick={onToggleCollapse}
          aria-label={collapsed ? 'Expand sidebar' : 'Collapse sidebar'}
          data-tip={collapsed ? 'Expand sidebar' : 'Collapse sidebar'}
          data-tip-right
        >
          {collapsed ? <PanelLeftOpen size={13} /> : <PanelLeftClose size={13} />}
        </button>

        <nav className="sidebar-nav">
          {sections.map((section) => (
            <div key={section.label}>
              <div className="sidebar-section">{section.label}</div>
              {section.links.map((link) => (
                <NavLink
                  key={link.to}
                  to={link.to}
                  end={link.end}
                  className={({ isActive }) => `sidebar-link ${isActive ? 'active' : ''}`}
                  onClick={onClose}
                  data-tip={collapsed ? link.label : undefined}
                  data-tip-right={collapsed ? '' : undefined}
                >
                  <link.icon size={17} />
                  <span>{link.label}</span>
                  {link.badge > 0 && <span className="link-badge">{link.badge}</span>}
                </NavLink>
              ))}
            </div>
          ))}
        </nav>
      </aside>
    </>
  )
}
