import { daysBetween, todayISO, formatDate } from '../../utils/format.js'

/**
 * Horizontal due-date timeline for active loans: each track runs from the
 * issue date to the due date with a "today" marker and a colour that turns
 * amber as the due date approaches (red once overdue).
 */
export default function LoanTimeline({ items }) {
  if (!items || items.length === 0) return null
  const today = todayISO()

  return (
    <section className="card card-pad mb-4 loan-timeline">
      <div className="row-between mb-3">
        <div>
          <div className="section-title" style={{ fontSize: 15 }}>Loan timeline</div>
          <div className="section-sub">Where each book sits between issue and due date</div>
        </div>
        <span className="text-xs text-muted">Today · {formatDate(today)}</span>
      </div>

      <div className="timeline-list">
        {items.map((item) => {
          const total = Math.max(1, daysBetween(item.issueDate, item.dueDate))
          const elapsed = daysBetween(item.issueDate, today)
          const progress = Math.min(100, Math.max(0, (elapsed / total) * 100))
          const left = daysBetween(today, item.dueDate)
          const tone = left < 0 ? 'danger' : left <= 7 ? 'warn' : 'ok'

          return (
            <div className="timeline-row" key={item.id}>
              <div className="timeline-head">
                <span className="timeline-title">{item.title}</span>
                <span className={`timeline-due ${tone}`}>
                  {left < 0
                    ? `${Math.abs(left)} day${Math.abs(left) === 1 ? '' : 's'} overdue`
                    : left === 0
                      ? 'Due today'
                      : `${left} day${left === 1 ? '' : 's'} left`}
                </span>
              </div>
              <div className={`timeline-track ${tone}`}>
                <div className="timeline-fill" style={{ width: `${progress}%` }} />
                <span className="timeline-today" style={{ left: `${progress}%` }} title="Today" />
              </div>
              <div className="timeline-dates">
                <span>{formatDate(item.issueDate)}</span>
                <span>{formatDate(item.dueDate)}</span>
              </div>
            </div>
          )
        })}
      </div>
    </section>
  )
}
