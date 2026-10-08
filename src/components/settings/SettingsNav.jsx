/**
 * Section navigator for the Settings pages.
 * Vertical list on desktop, horizontally scrollable pills on mobile.
 * Props: items [{ id, label, icon }], active, onChange(id).
 */
export default function SettingsNav({ items, active, onChange }) {
  return (
    <nav className="settings-nav" aria-label="Settings sections">
      {items.map((item) => {
        const Icon = item.icon
        return (
          <button
            key={item.id}
            type="button"
            className={`settings-nav-item${active === item.id ? ' active' : ''}`}
            onClick={() => onChange(item.id)}
            aria-current={active === item.id ? 'page' : undefined}
          >
            <Icon size={17} />
            <span>{item.label}</span>
          </button>
        )
      })}
    </nav>
  )
}