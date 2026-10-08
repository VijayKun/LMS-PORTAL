import { MonitorSmartphone } from 'lucide-react'
import { useTheme } from '../../store/ThemeContext.jsx'
import { useToast } from '../../store/ToastContext.jsx'
import { Field, Select } from '../ui/Field.jsx'
import SectionHead from './SectionHead.jsx'

/**
 * Appearance: reuses the app-wide theme system (ThemeContext) and adds
 * "System default" plus the interface density preference.
 */
export default function AppearanceSection() {
  const { mode, setMode, density, setDensity } = useTheme()
  const { toast } = useToast()

  function changeTheme(next) {
    setMode(next)
    toast(
      next === 'system' ? 'Theme now follows your system setting.' : 'Theme updated.',
      'success',
    )
  }

  function changeDensity(next) {
    setDensity(next)
    toast('Interface density updated.', 'success')
  }

  return (
    <>
      <SectionHead
        icon={MonitorSmartphone}
        tone="blue"
        title="Appearance"
        desc="Customise how the library portal looks and how much fits on screen."
      />
      <div className="form-row">
        <Field label="Theme">
          <Select value={mode} onChange={(e) => changeTheme(e.target.value)}>
            <option value="light">Light</option>
            <option value="dark">Dark</option>
            <option value="system">System default</option>
          </Select>
        </Field>
        <Field label="Interface density">
          <Select value={density} onChange={(e) => changeDensity(e.target.value)}>
            <option value="comfortable">Comfortable</option>
            <option value="compact">Compact</option>
          </Select>
        </Field>
      </div>
      <p className="text-xs text-muted" style={{ marginTop: 14 }}>
        Theme and density apply across the whole portal and are remembered on this
        device.
      </p>
    </>
  )
}