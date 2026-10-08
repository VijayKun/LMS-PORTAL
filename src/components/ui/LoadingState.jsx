/** Spinner + message, used while mock data "loads". */
export default function LoadingState({ label = 'Loading library data...' }) {
  return (
    <div className="loading-state">
      <div className="spinner" aria-hidden="true"></div>
      <p className="text-secondary">{label}</p>
    </div>
  )
}
