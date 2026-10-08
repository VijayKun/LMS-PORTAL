import { useState } from 'react'
import { Copy, Check } from 'lucide-react'

/**
 * Inline copy-to-clipboard text (ISBNs, transaction IDs...).
 * The copy button appears on hover/focus and flips to a check for a moment.
 */
export default function CopyText({ value, label, className = '' }) {
  const [copied, setCopied] = useState(false)

  async function copy() {
    try {
      await navigator.clipboard.writeText(String(value))
    } catch {
      // Clipboard API unavailable (http / older browsers) - legacy fallback.
      const ta = document.createElement('textarea')
      ta.value = String(value)
      ta.style.position = 'fixed'
      ta.style.opacity = '0'
      document.body.appendChild(ta)
      ta.select()
      try {
        document.execCommand('copy')
      } catch {
        /* copy not permitted - the value is still selectable on screen */
      }
      ta.remove()
    }
    setCopied(true)
    window.setTimeout(() => setCopied(false), 1600)
  }

  return (
    <span className={`copytext ${copied ? 'copied' : ''} ${className}`.trim()}>
      <span className="copytext-value">{value}</span>
      <button
        type="button"
        className="copytext-btn"
        onClick={copy}
        aria-label={`Copy ${label || value}`}
        data-tip={copied ? 'Copied!' : 'Copy'}
      >
        {copied ? <Check size={12} /> : <Copy size={12} />}
      </button>
    </span>
  )
}
