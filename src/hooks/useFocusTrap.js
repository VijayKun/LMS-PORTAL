import { useEffect, useRef } from 'react'

const FOCUSABLE =
  'a[href], button:not([disabled]), textarea:not([disabled]), input:not([disabled]), select:not([disabled]), [tabindex]:not([tabindex="-1"])'

/**
 * Traps keyboard focus inside `ref` while `active` is true.
 * - moves focus into the container on open (first focusable, else the node)
 * - Tab / Shift+Tab cycle within the container
 * - Escape calls `onEscape`
 * - restores focus to the previously focused element on close
 * - locks background scroll while active
 *
 * `onEscape` is kept in a ref so passing an inline arrow does not restart
 * the trap (which would yank focus back on every parent re-render).
 */
export function useFocusTrap(ref, active, onEscape) {
  const escapeRef = useRef(onEscape)

  useEffect(() => {
    escapeRef.current = onEscape
  }, [onEscape])

  useEffect(() => {
    if (!active || !ref.current) return
    const container = ref.current
    const previous = document.activeElement

    const first = container.querySelector(FOCUSABLE)
    ;(first || container).focus({ preventScroll: true })

    function onKeyDown(e) {
      if (e.key === 'Escape') {
        e.stopPropagation()
        escapeRef.current?.()
        return
      }
      if (e.key !== 'Tab') return
      const items = Array.from(container.querySelectorAll(FOCUSABLE)).filter(
        (el) => el.offsetParent !== null,
      )
      if (!items.length) {
        e.preventDefault()
        return
      }
      const firstItem = items[0]
      const lastItem = items[items.length - 1]
      if (e.shiftKey && document.activeElement === firstItem) {
        e.preventDefault()
        lastItem.focus()
      } else if (!e.shiftKey && document.activeElement === lastItem) {
        e.preventDefault()
        firstItem.focus()
      }
    }

    document.addEventListener('keydown', onKeyDown, true)
    const prevOverflow = document.body.style.overflow
    document.body.style.overflow = 'hidden'

    return () => {
      document.removeEventListener('keydown', onKeyDown, true)
      document.body.style.overflow = prevOverflow
      if (previous instanceof HTMLElement && previous.isConnected) previous.focus({ preventScroll: true })
    }
  }, [ref, active])
}
