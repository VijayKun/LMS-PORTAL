import {
  ResponsiveContainer,
  BarChart,
  Bar,
  LineChart,
  Line,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  Legend,
} from 'recharts'
import { TrendingUp, AlertTriangle } from 'lucide-react'
import { useLibrary } from '../../store/LibraryContext.jsx'
import { useTheme } from '../../store/ThemeContext.jsx'
import { usePageTitle } from '../../store/PageTitleContext.jsx'
import Badge from '../../components/ui/Badge.jsx'
import { adminStats, overdueLoans, daysRemaining } from '../../utils/selectors.js'
import { overdueBuckets } from '../../data/analytics.js'

/** Palette + axis/tooltip colours that follow the active theme. */
function chartTheme(dark) {
  if (dark) {
    return {
      blue: '#6B95F2',
      green: '#3ECF7E',
      amber: '#F5A524',
      red: '#F87171',
      grey: '#8FA0BC',
      pie: ['#6B95F2', '#3ECF7E', '#F5A524', '#8FA0BC', '#9AB4F7'],
      grid: '#233252',
      axis: '#7C8CAB',
      cursor: 'rgba(107, 149, 242, 0.08)',
      tipBg: '#1B2A45',
      tipBorder: '#2C3E60',
      text: '#E6EDF7',
      muted: '#9FB0CB',
    }
  }
  return {
    blue: '#3A49C9',
    green: '#16803C',
    amber: '#D97706',
    red: '#C62828',
    grey: '#9AA1B1',
    pie: ['#3A49C9', '#16803C', '#D97706', '#9AA1B1', '#6B7FD7'],
    grid: '#EEF0F3',
    axis: '#8B91A1',
    cursor: '#F4F5F8',
    tipBg: '#FFFFFF',
    tipBorder: '#E6E8EE',
    text: '#14213D',
    muted: '#55647F',
  }
}

function ChartCard({ title, subtitle, children, badge }) {
  return (
    <section className="card">
      <div className="card-header">
        <h3>{title}</h3>
        <div className="card-actions">{badge}</div>
      </div>
      <div className="card-body">
        {subtitle && <p className="text-xs text-muted mb-3">{subtitle}</p>}
        <div style={{ width: '100%', height: 260 }}>{children}</div>
      </div>
    </section>
  )
}

export default function Analytics() {
  usePageTitle('Analytics')
  const { state } = useLibrary()
  const { theme } = useTheme()

  const C = chartTheme(theme === 'dark')
  const axisProps = { fontSize: 12, fill: C.axis }
  const tooltipStyle = {
    contentStyle: {
      background: C.tipBg,
      border: `1px solid ${C.tipBorder}`,
      borderRadius: 10,
      fontSize: 13,
      color: C.text,
      boxShadow: '0 8px 24px -12px rgba(0,0,0,.4)',
    },
    labelStyle: { fontSize: 13, fontWeight: 600, color: C.text },
    itemStyle: { color: C.muted },
    cursor: { fill: C.cursor },
  }
  const legendStyle = { fontSize: 12, color: C.muted }

  const stats = adminStats(state)
  const overdue = overdueLoans(state)
  const activeStudents = state.analytics.activeStudents.map((s) => ({ name: s.name, books: s.borrows }))

  const overdueData = overdueBuckets.map((bucket) => ({
    ...bucket,
    count: overdue.filter((tx) => {
      const days = Math.abs(daysRemaining(tx.dueDate))
      if (bucket.label === '1-3 days') return days >= 1 && days <= 3
      if (bucket.label === '4-7 days') return days >= 4 && days <= 7
      if (bucket.label === '8-14 days') return days >= 8 && days <= 14
      return days >= 15
    }).length,
  }))

  return (
    <>
      <div className="page-header">
        <div>
          <h1>Analytics</h1>
          <p className="page-sub">Borrowing trends, popular titles and fine collection at a glance.</p>
        </div>
        <div className="page-actions">
          <Badge tone="primary">Mock data</Badge>
        </div>
      </div>

      <div className="grid grid-2 mb-4">
        <ChartCard title="Books borrowed over time" subtitle="Issues vs returns per month">
          <ResponsiveContainer width="100%" height="100%">
            <LineChart data={state.analytics.monthlyBorrowing}>
              <CartesianGrid strokeDasharray="3 3" stroke={C.grid} />
              <XAxis dataKey="month" {...axisProps} />
              <YAxis {...axisProps} width={34} />
              <Tooltip {...tooltipStyle} />
              <Legend wrapperStyle={legendStyle} />
              <Line type="monotone" dataKey="issued" stroke={C.blue} strokeWidth={2.5} dot={{ r: 3 }} />
              <Line type="monotone" dataKey="returned" stroke={C.green} strokeWidth={2.5} dot={{ r: 3 }} />
            </LineChart>
          </ResponsiveContainer>
        </ChartCard>

        <ChartCard title="Most borrowed books" subtitle="All-time borrow count per title">
          <ResponsiveContainer width="100%" height="100%">
            <BarChart data={state.analytics.popularBooks} layout="vertical" margin={{ left: 8 }}>
              <CartesianGrid strokeDasharray="3 3" stroke={C.grid} horizontal={false} />
              <XAxis type="number" {...axisProps} />
              <YAxis type="category" dataKey="title" width={150} {...axisProps} />
              <Tooltip {...tooltipStyle} />
              <Bar dataKey="borrows" fill={C.blue} radius={[0, 6, 6, 0]} barSize={16} />
            </BarChart>
          </ResponsiveContainer>
        </ChartCard>

        <ChartCard title="Fine collection" subtitle="Collected vs pending per month">
          <ResponsiveContainer width="100%" height="100%">
            <BarChart data={state.analytics.fineTrend}>
              <CartesianGrid strokeDasharray="3 3" stroke={C.grid} />
              <XAxis dataKey="month" {...axisProps} />
              <YAxis {...axisProps} width={40} />
              <Tooltip {...tooltipStyle} />
              <Legend wrapperStyle={legendStyle} />
              <Bar dataKey="collected" name="Collected (₹)" fill={C.green} radius={[6, 6, 0, 0]} barSize={20} />
              <Bar dataKey="pending" name="Pending (₹)" fill={C.red} radius={[6, 6, 0, 0]} barSize={20} />
            </BarChart>
          </ResponsiveContainer>
        </ChartCard>

        <ChartCard title="Overdue books by age" subtitle="How late the current overdue loans are">
          <ResponsiveContainer width="100%" height="100%">
            <BarChart data={overdueData}>
              <CartesianGrid strokeDasharray="3 3" stroke={C.grid} />
              <XAxis dataKey="label" {...axisProps} />
              <YAxis {...axisProps} width={30} allowDecimals={false} />
              <Tooltip {...tooltipStyle} />
              <Bar dataKey="count" name="Books" fill={C.red} radius={[6, 6, 0, 0]} barSize={34} />
            </BarChart>
          </ResponsiveContainer>
        </ChartCard>
      </div>

      <div className="grid grid-split" style={{ gridTemplateColumns: 'minmax(0, 1fr) minmax(0, 1fr)' }}>
        <section className="card">
          <div className="card-header">
            <h3><TrendingUp size={16} style={{ verticalAlign: -3, marginRight: 8 }} />Most active students</h3>
          </div>
          <div className="card-body stack stack-md">
            {activeStudents.map((s) => {
              const max = activeStudents[0].books
              return (
                <div key={s.name}>
                  <div className="row-between text-sm mb-2">
                    <span className="font-semibold">{s.name}</span>
                    <span className="text-muted">{s.books} books</span>
                  </div>
                  <div className="progress-track">
                    <div className="progress-fill" style={{ width: `${(s.books / max) * 100}%` }} />
                  </div>
                </div>
              )
            })}
          </div>
        </section>

        <section className="card">
          <div className="card-header">
            <h3><AlertTriangle size={16} style={{ verticalAlign: -3, marginRight: 8 }} />Library health</h3>
          </div>
          <div className="card-body">
            <div className="info-list">
              <div className="info-row">
                <span className="info-label">Total copies</span>
                <span className="info-value">{stats.totalCopies}</span>
              </div>
              <div className="info-row">
                <span className="info-label">Currently issued</span>
                <span className="info-value">{stats.issuedCopies}</span>
              </div>
              <div className="info-row">
                <span className="info-label">Utilisation rate</span>
                <span className="info-value">
                  {Math.round((stats.issuedCopies / Math.max(1, stats.totalCopies)) * 100)}%
                </span>
              </div>
              <div className="info-row">
                <span className="info-label">Overdue loans</span>
                <span className="info-value text-danger">{stats.overdue}</span>
              </div>
              <div className="info-row">
                <span className="info-label">Active reservations</span>
                <span className="info-value">{stats.activeReservations}</span>
              </div>
              <div className="info-row">
                <span className="info-label">Fines collected</span>
                <span className="info-value text-success">₹{stats.collectedFines}</span>
              </div>
              <div className="info-row">
                <span className="info-label">Fines pending</span>
                <span className="info-value text-danger">₹{stats.pendingFines}</span>
              </div>
            </div>
          </div>
        </section>
      </div>
    </>
  )
}
