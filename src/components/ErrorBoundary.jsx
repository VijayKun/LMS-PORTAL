import { Component } from 'react'

/**
 * Catches render-time crashes so a broken page never takes down the whole app.
 * Shows a friendly card with a reload option and the error message for debugging.
 */
export default class ErrorBoundary extends Component {
  constructor(props) {
    super(props)
    this.state = { error: null }
  }

  static getDerivedStateFromError(error) {
    return { error }
  }

  componentDidCatch(error, info) {
    console.error('Page crashed:', error, info)
  }

  render() {
    if (this.state.error) {
      return (
        <div className="empty-state" role="alert">
          <div className="empty-icon">⚠️</div>
          <h3>This page hit a snag</h3>
          <p className="text-muted">Something went wrong while rendering. Your data is safe.</p>
          <p className="text-xs text-muted" style={{ maxWidth: 480, wordBreak: 'break-word' }}>
            {String(this.state.error?.message || this.state.error)}
          </p>
          <div style={{ display: 'flex', gap: 10, marginTop: 14 }}>
            <button className="btn btn-primary" onClick={() => window.location.reload()}>
              Reload app
            </button>
            <button className="btn" onClick={() => this.setState({ error: null })}>
              Try again
            </button>
          </div>
        </div>
      )
    }
    return this.props.children
  }
}
