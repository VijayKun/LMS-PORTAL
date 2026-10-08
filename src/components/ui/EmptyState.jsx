import EmptyIllustration from './EmptyIllustration.jsx'

/**
 * Friendly placeholder when a list has nothing to show.
 * Pass `kind` ("search" | "books" | "bell" | "default") for a full
 * illustration, or `icon` for the original compact icon tile.
 */
export default function EmptyState({ icon: Icon, kind, title, message, action, children }) {
  return (
    <div className="empty-state">
      {kind ? (
        <div className="empty-art" aria-hidden="true">
          <EmptyIllustration kind={kind} />
        </div>
      ) : (
        <div className="empty-icon">{Icon && <Icon size={24} />}</div>
      )}
      <h3>{title}</h3>
      {message && <p>{message}</p>}
      {action}
      {children}
    </div>
  )
}
