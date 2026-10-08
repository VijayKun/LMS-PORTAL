/**
 * Mobile card layout for data tables.
 *
 * CSS (index.css, <=768px) turns each table row into a card and prints a
 * per-cell label via `td::before { content: attr(data-label) }`. This module
 * fills those labels from the table's own <thead> and keeps them fresh as
 * React re-renders rows (pagination, filtering, sorting).
 *
 * Returns a cleanup function for useEffect.
 */
const TABLE_SELECTOR = '.table-responsive .table, .table-responsive table.table'

function applyLabels(table) {
  const heads = Array.from(table.querySelectorAll('thead th')).map((th) =>
    th.textContent.trim(),
  )
  if (!heads.length) return
  table.querySelectorAll('tbody tr').forEach((row) => {
    Array.from(row.children).forEach((cell, index) => {
      if (cell.tagName !== 'TD' || cell.colSpan > 1) return
      const label = heads[index]
      if (label && cell.dataset.label !== label) cell.dataset.label = label
    })
  })
}

export function enhanceTables(root) {
  if (!root) return () => {}
  let frame = 0
  const run = () => {
    cancelAnimationFrame(frame)
    frame = requestAnimationFrame(() => {
      root.querySelectorAll(TABLE_SELECTOR).forEach(applyLabels)
    })
  }
  run()
  // childList only: writing dataset.label does not retrigger the observer.
  const observer = new MutationObserver(run)
  observer.observe(root, { childList: true, subtree: true })
  return () => {
    cancelAnimationFrame(frame)
    observer.disconnect()
  }
}
