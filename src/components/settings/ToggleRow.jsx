import Switch from '../ui/Switch.jsx'

/**
 * Label + hint on the left, toggle switch on the right.
 * Standard row used by every notification/preference list in Settings.
 */
export default function ToggleRow({ label, hint, checked, onChange, disabled }) {
  return (
    <div className="setting-row">
      <div>
        <div className="setting-label">{label}</div>
        {hint && <div className="setting-hint">{hint}</div>}
      </div>
      <Switch checked={!!checked} onChange={onChange} disabled={disabled} label={label} />
    </div>
  )
}