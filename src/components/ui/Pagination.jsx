import { ChevronLeft, ChevronRight } from 'lucide-react'

/**
 * Simple numbered pagination.
 * Props: page, totalPages, totalItems, pageSize, onPage
 */
export default function Pagination({ page, totalPages, totalItems, pageSize = 8, onPage }) {
  if (totalPages <= 1) return null

  const from = totalItems === 0 ? 0 : (page - 1) * pageSize + 1
  const to = Math.min(page * pageSize, totalItems)
  const numbers = []
  for (let i = 1; i <= totalPages; i++) {
    if (i === 1 || i === totalPages || Math.abs(i - page) <= 1) numbers.push(i)
    else if (numbers[numbers.length - 1] !== '…') numbers.push('…')
  }

  return (
    <div className="pagination">
      <span className="page-info">
        Showing {from}–{to} of {totalItems}
      </span>
      <button
        className="page-btn"
        disabled={page === 1}
        onClick={() => onPage(page - 1)}
        aria-label="Previous page"
      >
        <ChevronLeft size={15} />
      </button>
      {numbers.map((n, idx) =>
        n === '…' ? (
          <span key={`gap-${idx}`} className="page-info">…</span>
        ) : (
          <button
            key={n}
            className={`page-btn ${n === page ? 'active' : ''}`}
            onClick={() => onPage(n)}
          >
            {n}
          </button>
        ),
      )}
      <button
        className="page-btn"
        disabled={page === totalPages}
        onClick={() => onPage(page + 1)}
        aria-label="Next page"
      >
        <ChevronRight size={15} />
      </button>
    </div>
  )
}
