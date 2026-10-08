import { useEffect, useRef, useState } from 'react'

const prefersReduced = () =>
  typeof window !== 'undefined' &&
  window.matchMedia('(prefers-reduced-motion: reduce)').matches

/** Animates a number from its previous value to the new one (ease-out). */
function useCountUp(target) {
  const [display, setDisplay] = useState(() => (prefersReduced() ? target : 0))
  const fromRef = useRef(prefersReduced() ? target : 0)
  const rafRef = useRef(0)

  useEffect(() => {
    if (prefersReduced()) {
      // Deferred so the effect body never sets state synchronously.
      const id = requestAnimationFrame(() => {
        setDisplay(target)
        fromRef.current = target
      })
      return () => cancelAnimationFrame(id)
    }
    const from = fromRef.current
    if (from === target) return undefined
    const start = performance.now()
    const duration = 750
    const tick = (now) => {
      const t = Math.min(1, (now - start) / duration)
      const eased = 1 - Math.pow(1 - t, 3)
      const current = Math.round(from + (target - from) * eased)
      setDisplay(current)
      if (t < 1) rafRef.current = requestAnimationFrame(tick)
      else fromRef.current = target
    }
    rafRef.current = requestAnimationFrame(tick)
    return () => cancelAnimationFrame(rafRef.current)
  }, [target])

  return display
}

/** Splits '12 Books' / '₹4,500' into prefix, number and suffix. */
function splitValue(value) {
  if (typeof value === 'number') return { prefix: '', num: value, suffix: '' }
  const match = String(value).match(/^(.*?)(\d[\d,]*)(\s*\D.*)?$/)
  if (!match) return null
  const num = Number(match[2].replace(/,/g, ''))
  if (Number.isNaN(num)) return null
  return { prefix: match[1] || '', num, suffix: match[3] || '' }
}

const SPARK_COLORS = {
  blue: 'var(--primary)',
  green: 'var(--success)',
  amber: 'var(--warning)',
  red: 'var(--danger)',
  cyan: 'var(--info)',
}

/** Mini trend line drawn from a numeric series. */
function Sparkline({ data, tone }) {
  if (!Array.isArray(data) || data.length < 2) return null
  const w = 96
  const h = 30
  const pad = 3
  const min = Math.min(...data)
  const max = Math.max(...data)
  const range = max - min || 1
  const points = data.map((v, i) => [
    pad + (i * (w - pad * 2)) / (data.length - 1),
    h - pad - ((v - min) / range) * (h - pad * 2),
  ])
  const line = points
    .map(([x, y], i) => `${i ? 'L' : 'M'}${x.toFixed(1)} ${y.toFixed(1)}`)
    .join(' ')
  const area = `${line} L${points[points.length - 1][0].toFixed(1)} ${h} L${points[0][0].toFixed(1)} ${h} Z`
  const color = SPARK_COLORS[tone] || SPARK_COLORS.blue
  const [lx, ly] = points[points.length - 1]

  return (
    <svg
      className="stat-spark"
      viewBox={`0 0 ${w} ${h}`}
      width={w}
      height={h}
      aria-hidden="true"
      preserveAspectRatio="none"
    >
      <path d={area} fill={color} opacity="0.12" />
      <path d={line} fill="none" stroke={color} strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" />
      <circle cx={lx} cy={ly} r="2.5" fill={color} />
    </svg>
  )
}

function TrendChip({ pct, dir, good }) {
  const up = dir !== 'down'
  const cls = up
    ? good
      ? 'trend up good'
      : 'trend up bad'
    : good
      ? 'trend down bad'
      : 'trend down good'
  return (
    <span className={cls}>
      <span className="trend-arrow" aria-hidden="true">{up ? '▲' : '▼'}</span>
      {Math.abs(pct)}%
    </span>
  )
}

/**
 * Dashboard metric card: icon + label + animated value + optional note,
 * trend chip (▲/▼ vs previous period) and a sparkline series.
 */
export default function StatCard({ icon: Icon, tone = 'blue', label, value, note, trend, spark }) {
  const parts = splitValue(value)
  const counted = useCountUp(parts ? parts.num : 0)
  const shown = parts
    ? `${parts.prefix}${counted.toLocaleString('en-IN')}${parts.suffix}`
    : value

  return (
    <div className="stat-card">
      <div className={`stat-icon ${tone}`}>
        {Icon && <Icon size={19} />}
      </div>
      <div className="flex-1">
        <div className="stat-label">{label}</div>
        <div className="stat-value-row">
          <div className="stat-value">{shown}</div>
          {trend && <TrendChip {...trend} />}
        </div>
        {note && <div className="stat-note">{note}</div>}
        {spark && <Sparkline data={spark} tone={tone} />}
      </div>
    </div>
  )
}
