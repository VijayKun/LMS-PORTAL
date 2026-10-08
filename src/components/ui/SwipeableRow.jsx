import { useRef, useState } from 'react'

/**
 * Touch wrapper that reveals a row of actions when the content is swiped
 * left (phones only in practice - mouse events are ignored). Tapping the
 * content again, or swiping right, closes it.
 *
 *   <SwipeableRow actions={<button…>Return</button>}>
 *     <div className="loan-card">…</div>
 *   </SwipeableRow>
 */
export default function SwipeableRow({ actions, children, revealWidth = 168 }) {
  const [dx, setDx] = useState(0)
  const [dragging, setDragging] = useState(false)
  const start = useRef(null) // { x, y, open }

  function onTouchStart(e) {
    const t = e.touches[0]
    start.current = { x: t.clientX, y: t.clientY, open: dx < 0 }
  }

  function onTouchMove(e) {
    if (!start.current) return
    const t = e.touches[0]
    const rawX = t.clientX - start.current.x
    const rawY = t.clientY - start.current.y
    // Let the page scroll when the gesture is mostly vertical.
    if (Math.abs(rawY) > Math.abs(rawX) && Math.abs(rawY) > 8) {
      start.current = null
      setDragging(false)
      return
    }
    if (!dragging) setDragging(true)
    const base = start.current.open ? -revealWidth : 0
    setDx(Math.max(-revealWidth - 12, Math.min(0, base + rawX)))
  }

  function onTouchEnd() {
    if (!start.current) return
    start.current = null
    setDragging(false)
    setDx((current) => (current < -revealWidth / 2 ? -revealWidth : 0))
  }

  function close() {
    if (dx < 0) setDx(0)
  }

  // While open, the first tap just closes the row instead of activating
  // whatever button sits underneath.
  function onClickCapture(e) {
    if (dx < 0) {
      e.preventDefault()
      e.stopPropagation()
      close()
    }
  }

  return (
    <div className={`swipe-row ${dragging ? 'dragging' : ''}`}>
      <div className="swipe-actions" style={{ minWidth: revealWidth }} aria-hidden={dx === 0}>
        {actions}
      </div>
      <div
        className="swipe-card"
        style={{ transform: `translateX(${dx}px)` }}
        onTouchStart={onTouchStart}
        onTouchMove={onTouchMove}
        onTouchEnd={onTouchEnd}
        onClickCapture={onClickCapture}
      >
        {children}
      </div>
    </div>
  )
}
