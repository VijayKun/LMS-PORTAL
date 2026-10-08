import { useId, useRef } from 'react'
import { X } from 'lucide-react'
import { useFocusTrap } from '../../hooks/useFocusTrap.js'

/**
 * Generic modal window.
 * Props: open, title, onClose, children, footer, size ('sm' | 'lg')
 */
export default function Modal({ open, title, onClose, children, footer, size = 'sm' }) {
  const dialogRef = useRef(null)
  const titleId = useId()

  useFocusTrap(dialogRef, open, onClose)

  if (!open) return null

  return (
    <div className="modal-overlay" onMouseDown={(e) => e.target === e.currentTarget && onClose?.()}>
      <div
        className={`modal ${size === 'lg' ? 'modal-lg' : ''}`}
        role="dialog"
        aria-modal="true"
        aria-labelledby={titleId}
        ref={dialogRef}
        tabIndex={-1}
      >
        <div className="modal-header">
          <h3 id={titleId}>{title}</h3>
          <button className="icon-btn modal-close" onClick={onClose} aria-label="Close dialog">
            <X size={17} />
          </button>
        </div>
        <div className="modal-body">{children}</div>
        {footer && <div className="modal-footer">{footer}</div>}
      </div>
    </div>
  )
}
