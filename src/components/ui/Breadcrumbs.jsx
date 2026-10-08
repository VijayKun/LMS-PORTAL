import { Link } from 'react-router-dom'
import { ChevronRight } from 'lucide-react'

/**
 * Breadcrumb trail for nested pages.
 * items: [{ label, to? }] - last item is the current page (no link).
 */
export default function Breadcrumbs({ items }) {
  if (!items?.length) return null
  return (
    <nav className="breadcrumbs no-print" aria-label="Breadcrumb">
      {items.map((item, index) => {
        const last = index === items.length - 1
        return (
          <span className="crumb" key={`${item.label}-${index}`}>
            {index > 0 && <ChevronRight size={13} aria-hidden="true" className="crumb-sep" />}
            {last || !item.to ? (
              <span aria-current={last ? 'page' : undefined}>{item.label}</span>
            ) : (
              <Link to={item.to}>{item.label}</Link>
            )}
          </span>
        )
      })}
    </nav>
  )
}
