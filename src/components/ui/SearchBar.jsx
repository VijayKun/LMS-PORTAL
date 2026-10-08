import { Search } from 'lucide-react'

/** Search input used on list pages. */
export default function SearchBar({ value, onChange, placeholder = 'Search...', className = '' }) {
  return (
    <div className={`searchbar ${className}`.trim()}>
      <Search size={16} />
      <input
        type="search"
        className="input"
        value={value}
        placeholder={placeholder}
        onChange={(e) => onChange(e.target.value)}
        aria-label={placeholder}
      />
    </div>
  )
}
