import { Lock, ShieldCheck } from 'lucide-react'
import { Field, Select } from '../ui/Field.jsx'
import { PRIVACY_OPTIONS } from '../../data/settings.js'
import SectionHead from './SectionHead.jsx'
import ToggleRow from './ToggleRow.jsx'

/**
 * Simple privacy controls. `privacy` = { profileVisibility, showContact }.
 * onChange receives the next privacy object so the caller can persist it.
 */
export default function PrivacySection({ privacy, onChange }) {
  return (
    <>
      <SectionHead
        icon={Lock}
        tone="green"
        title="Privacy"
        desc="Control who can see your profile details."
      />
      <Field label="Profile visibility" htmlFor="settings-privacy-visibility">
        <Select
          id="settings-privacy-visibility"
          value={privacy.profileVisibility}
          onChange={(e) => onChange({ ...privacy, profileVisibility: e.target.value })}
        >
          {PRIVACY_OPTIONS.map((o) => (
            <option key={o.value} value={o.value}>
              {o.label}
            </option>
          ))}
        </Select>
      </Field>
      <ToggleRow
        label="Show contact information"
        hint="Display your phone number to other members in the library directory."
        checked={privacy.showContact}
        onChange={(v) => onChange({ ...privacy, showContact: v })}
      />
      <div className="settings-note" style={{ marginTop: 16 }}>
        <ShieldCheck size={17} />
        <span>
          Your data is stored on this device for the demo. Once the library server
          is connected, privacy choices will be enforced by the backend.
        </span>
      </div>
    </>
  )
}