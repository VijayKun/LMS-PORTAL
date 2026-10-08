import { AlertTriangle } from 'lucide-react'
import Modal from './Modal.jsx'

/**
 * Confirmation dialog for destructive actions.
 * Props: open, title, message, confirmLabel, onConfirm, onClose, danger
 */
export default function ConfirmDialog({
  open,
  title = 'Are you sure?',
  message,
  confirmLabel = 'Confirm',
  onConfirm,
  onClose,
  danger = true,
}) {
  return (
    <Modal
      open={open}
      title={title}
      onClose={onClose}
      footer={
        <>
          <button className="btn btn-secondary" onClick={onClose}>
            Cancel
          </button>
          <button className={`btn ${danger ? 'btn-danger' : 'btn-primary'}`} onClick={onConfirm}>
            {confirmLabel}
          </button>
        </>
      }
    >
      <div className={`confirm-icon ${danger ? 'danger' : 'info'}`}>
        <AlertTriangle size={21} />
      </div>
      <p className="text-secondary">{message}</p>
    </Modal>
  )
}
