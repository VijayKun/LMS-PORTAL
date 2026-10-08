import { useEffect, useState } from 'react'
import { ArrowUp } from 'lucide-react'

/**
 * Floating "back to top" button. Appears after scrolling one viewport down,
 * hidden on print and while the mobile drawer is open.
 */
export default function BackToTop() {
  const [visible, setVisible] = useState(false)

  useEffect(() => {
    function onScroll() {
      setVisible(window.scrollY > 420)
    }
    onScroll()
    window.addEventListener('scroll', onScroll, { passive: true })
    return () => window.removeEventListener('scroll', onScroll)
  }, [])

  if (!visible) return null

  return (
    <button
      className="back-to-top no-print"
      onClick={() => window.scrollTo({ top: 0, behavior: 'smooth' })}
      aria-label="Back to top"
      data-tip="Back to top"
    >
      <ArrowUp size={17} />
    </button>
  )
}
