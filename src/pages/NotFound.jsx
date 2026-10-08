import { Link } from 'react-router-dom'
import { Compass } from 'lucide-react'

export default function NotFoundPage() {
  return (
    <div style={{ minHeight: '100vh', display: 'grid', placeItems: 'center', padding: 24 }}>
      <div className="empty-state">
        <div className="empty-icon">
          <Compass size={24} />
        </div>
        <h3>Page not found</h3>
        <p>The page you are looking for does not exist or has been moved.</p>
        <Link className="btn btn-primary" to="/login">
          Back to login
        </Link>
      </div>
    </div>
  )
}
