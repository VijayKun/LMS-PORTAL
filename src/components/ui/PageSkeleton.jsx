import Skeleton from './Skeleton.jsx'

/**
 * Full-page skeleton shown while a lazy route chunk loads:
 * mimics a topbar + stat cards + a content card.
 */
export default function PageSkeleton() {
  return (
    <div className="page-skeleton" role="status" aria-label="Loading page">
      <div className="skeleton-row">
        <Skeleton width={220} height={26} />
        <Skeleton width={120} height={38} radius={10} />
      </div>
      <div className="skeleton-grid">
        <Skeleton height={96} radius={14} />
        <Skeleton height={96} radius={14} />
        <Skeleton height={96} radius={14} />
        <Skeleton height={96} radius={14} />
      </div>
      <Skeleton height={280} radius={16} />
      <span className="sr-only">Loading…</span>
    </div>
  )
}
