import { Languages } from 'lucide-react'
import { Field, Select } from '../ui/Field.jsx'
import SectionHead from './SectionHead.jsx'

/**
 * Language preference. Only updates the saved preference for now - a full
 * translation system is not part of this build.
 */
export default function LanguageSection({ value, onChange }) {
  return (
    <>
      <SectionHead
        icon={Languages}
        tone="cyan"
        title="Language"
        desc="Choose the language for the portal interface."
      />
      <Field label="Interface language" htmlFor="settings-language">
        <Select
          id="settings-language"
          value={value}
          onChange={(e) => onChange(e.target.value)}
        >
          <option value="en">English</option>
          <option value="hi">Hindi (हिन्दी)</option>
        </Select>
      </Field>
      <p className="text-xs text-muted" style={{ marginTop: 10 }}>
        English translations are active now; Hindi will arrive with the backend
        release.
      </p>
    </>
  )
}