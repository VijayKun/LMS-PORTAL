/**
 * Accessible toggle switch used across the Settings pages.
 * Props: checked, onChange(nextValue), disabled, label (aria-label).
 */
export default function Switch({ checked = false, onChange, disabled = false, label }) {
  return (
    <button
      type="button"
      role="switch"
      aria-checked={checked}
      aria-label={label}
      disabled={disabled}
      className={`switch${checked ? ' on' : ''}`}
      onClick={() => onChange(!checked)}
    >
      <span className="knob" aria-hidden="true" />
    </button>
  )
}