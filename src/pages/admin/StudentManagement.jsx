import { useMemo, useState } from 'react'
import { useNavigate } from 'react-router-dom'
import { Plus, Eye, Pencil, Trash2, SearchX } from 'lucide-react'
import { useLibrary } from '../../store/LibraryContext.jsx'
import { usePageTitle } from '../../store/PageTitleContext.jsx'
import { EMAIL_DOMAINS } from '../../store/AuthContext.jsx'
import { useToast } from '../../store/ToastContext.jsx'
import SearchBar from '../../components/ui/SearchBar.jsx'
import { Select, Field, Input } from '../../components/ui/Field.jsx'
import Badge from '../../components/ui/Badge.jsx'
import Pagination from '../../components/ui/Pagination.jsx'
import EmptyState from '../../components/ui/EmptyState.jsx'
import Modal from '../../components/ui/Modal.jsx'
import ConfirmDialog from '../../components/ui/ConfirmDialog.jsx'
import { getInitials } from '../../utils/format.js'
import {
  activeLoansOf,
  reservationsOf,
  outstandingFines,
  totalOf,
} from '../../utils/selectors.js'
import { DEFAULT_PAGE_SIZE } from '../../data/constants.js'

const BLANK_STUDENT = {
  name: '',
  email: '',
  studentId: '',
  branch: 'Computer Science',
  program: 'B.Tech',
  semester: 1,
  contact: '',
  joinedDate: '2026-10-07',
  status: 'active',
}

/** Academic year derived from the semester count (2 semesters = 1 year). */
const yearOf = (s) => Math.ceil((Number(s.semester) || 1) / 2)

function yearLabel(y) {
  if (y === 1) return '1st year'
  if (y === 2) return '2nd year'
  if (y === 3) return '3rd year'
  return `${y}th year`
}

export default function StudentManagement() {
  usePageTitle('Student Management')
  const { state, addStudent, updateStudent, deleteStudent } = useLibrary()
  const { toast } = useToast()
  const navigate = useNavigate()

  const [query, setQuery] = useState('')
  const [branch, setBranch] = useState('all')
  const [year, setYear] = useState('all')
  const [semester, setSemester] = useState('all')
  const [status, setStatus] = useState('all')
  const [page, setPage] = useState(1)
  const [adding, setAdding] = useState(false)
  const [form, setForm] = useState({ ...BLANK_STUDENT })
  const [errors, setErrors] = useState({})
  const [deleting, setDeleting] = useState(null)

  const branches = useMemo(() => [...new Set(state.students.map((s) => s.branch))].sort(), [state.students])
  const years = useMemo(
    () => [...new Set(state.students.map(yearOf))].sort((a, b) => a - b),
    [state.students],
  )
  const semesters = useMemo(
    () => [...new Set(state.students.map((s) => Number(s.semester) || 1))].sort((a, b) => a - b),
    [state.students],
  )

  const filtered = useMemo(() => {
    const q = query.trim().toLowerCase()
    return state.students.filter((s) => {
      if (q) {
        const haystack = `${s.name} ${s.email} ${s.studentId} ${s.branch} ${s.program} ${yearLabel(yearOf(s))} semester ${s.semester}`
        if (!haystack.toLowerCase().includes(q)) return false
      }
      if (branch !== 'all' && s.branch !== branch) return false
      if (year !== 'all' && yearOf(s) !== Number(year)) return false
      if (semester !== 'all' && (Number(s.semester) || 1) !== Number(semester)) return false
      if (status !== 'all' && s.status !== status) return false
      return true
    })
  }, [state.students, query, branch, year, semester, status])

  const totalPages = Math.max(1, Math.ceil(filtered.length / DEFAULT_PAGE_SIZE))
  const currentPage = Math.min(page, totalPages)
  const rows = filtered.slice((currentPage - 1) * DEFAULT_PAGE_SIZE, currentPage * DEFAULT_PAGE_SIZE)

  /** Group the visible page by branch so each branch reads as its own block. */
  const groups = useMemo(() => {
    const map = new Map()
    rows.forEach((s) => {
      const key = s.branch || 'Unassigned'
      if (!map.has(key)) map.set(key, [])
      map.get(key).push(s)
    })
    return [...map.entries()].sort((a, b) => a[0].localeCompare(b[0]))
  }, [rows])

  function openAdd() {
    setForm({ ...BLANK_STUDENT })
    setErrors({})
    setAdding(true)
  }

  function saveStudent(e) {
    e.preventDefault()
    const errs = {}
    if (!form.name.trim()) errs.name = 'Name is required.'
    if (!/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(form.email)) errs.email = 'Valid email is required.'
    else if (!form.email.toLowerCase().endsWith(`@${EMAIL_DOMAINS.student}`))
      errs.email = `Student email must end with @${EMAIL_DOMAINS.student}.`
    if (!form.studentId.trim()) errs.studentId = 'Student ID is required.'
    else if (state.students.some((s) => s.studentId.toLowerCase() === form.studentId.trim().toLowerCase()))
      errs.studentId = 'This student ID already exists.'
    if (!/^[+\d][\d\s-]{7,}$/.test(form.contact)) errs.contact = 'Valid contact number is required.'
    setErrors(errs)
    if (Object.keys(errs).length) return

    addStudent({ ...form, semester: Number(form.semester) })
    setAdding(false)
    toast(`${form.name} added successfully.`, 'success')
  }

  function confirmDelete() {
    deleteStudent(deleting.id)
    toast(`${deleting.name} removed from the library.`, 'success')
    setDeleting(null)
  }

  function fineOf(studentId) {
    return totalOf(outstandingFines(state, studentId))
  }

  const hasFilters =
    query || branch !== 'all' || year !== 'all' || semester !== 'all' || status !== 'all'

  function clearFilters() {
    setQuery('')
    setBranch('all')
    setYear('all')
    setSemester('all')
    setStatus('all')
    setPage(1)
  }

  return (
    <>
      <div className="page-header">
        <div>
          <h1>Student Management</h1>
          <p className="page-sub">{state.students.length} registered borrowers</p>
        </div>
        <div className="page-actions">
          <button className="btn btn-primary" onClick={openAdd}>
            <Plus size={16} /> Add Student
          </button>
        </div>
      </div>

      <div className="card card-pad mb-4">
        <SearchBar value={query} onChange={(v) => { setQuery(v); setPage(1) }} placeholder="Search name, ID, branch, 2nd year, semester..." />
        <div className="filter-bar mt-3">
          <Select value={branch} onChange={(e) => { setBranch(e.target.value); setPage(1) }} aria-label="Branch">
            <option value="all">All branches</option>
            {branches.map((b) => <option key={b} value={b}>{b}</option>)}
          </Select>
          <Select value={year} onChange={(e) => { setYear(e.target.value); setPage(1) }} aria-label="Year">
            <option value="all">All years</option>
            {years.map((y) => <option key={y} value={y}>{yearLabel(y)}</option>)}
          </Select>
          <Select value={semester} onChange={(e) => { setSemester(e.target.value); setPage(1) }} aria-label="Semester">
            <option value="all">All semesters</option>
            {semesters.map((n) => <option key={n} value={n}>Semester {n}</option>)}
          </Select>
          <Select value={status} onChange={(e) => { setStatus(e.target.value); setPage(1) }} aria-label="Status">
            <option value="all">All statuses</option>
            <option value="active">Active</option>
            <option value="suspended">Suspended</option>
          </Select>
          {hasFilters && (
            <button className="btn btn-ghost btn-sm" onClick={clearFilters}>
              Clear filters
            </button>
          )}
        </div>
      </div>

      <div className="card">
        <div className="card-body flush">
          {rows.length === 0 ? (
            <EmptyState
              icon={SearchX}
              title="No students found"
              message="No students match the current search and filters."
              action={<button className="btn btn-primary" onClick={clearFilters}>Clear filters</button>}
            />
          ) : (
            groups.map(([group, list]) => (
              <section className="branch-group" key={group}>
                <header className="branch-group-head">
                  <span className="branch-group-name">{group}</span>
                  <span className="branch-group-count">
                    {list.length} {list.length === 1 ? 'student' : 'students'}
                  </span>
                </header>
                <div className="table-responsive">
                  <table className="table">
                    <thead>
                      <tr>
                        <th>Student ID</th>
                        <th>Name</th>
                        <th>Email</th>
                        <th>Books Issued</th>
                        <th>Reservations</th>
                        <th>Fine</th>
                        <th>Status</th>
                        <th className="text-right">Actions</th>
                      </tr>
                    </thead>
                    <tbody>
                      {list.map((s) => {
                        const fine = fineOf(s.id)
                        return (
                          <tr key={s.id}>
                            <td className="strong">{s.studentId}</td>
                            <td>
                              <div className="row" style={{ gap: 9 }}>
                                <div className="avatar avatar-sm">{getInitials(s.name)}</div>
                                <span className="font-semibold">{s.name}</span>
                              </div>
                            </td>
                            <td className="text-sm">{s.email}</td>
                            <td>{activeLoansOf(state, s.id).length}</td>
                            <td>{reservationsOf(state, s.id).filter((r) => r.status === 'waiting' || r.status === 'ready').length}</td>
                            <td>{fine > 0 ? <span className="text-danger font-bold">₹{fine}</span> : '—'}</td>
                            <td><Badge status={s.status}>{s.status === 'active' ? 'Active' : 'Suspended'}</Badge></td>
                            <td>
                              <div className="actions-cell">
                                <button className="icon-btn" title="View" onClick={() => navigate(`/admin/students/${s.id}`)} aria-label={`View ${s.name}`}>
                                  <Eye size={15} />
                                </button>
                                <button
                                  className="icon-btn"
                                  title="Suspend / activate"
                                  onClick={() => {
                                    updateStudent({ id: s.id, status: s.status === 'active' ? 'suspended' : 'active' })
                                    toast(`${s.name} ${s.status === 'active' ? 'suspended' : 'activated'}.`, 'info')
                                  }}
                                  aria-label={`Change status of ${s.name}`}
                                >
                                  <Pencil size={15} />
                                </button>
                                <button
                                  className="icon-btn"
                                  title="Remove"
                                  onClick={() => setDeleting(s)}
                                  aria-label={`Remove ${s.name}`}
                                  style={{ color: 'var(--danger)' }}
                                >
                                  <Trash2 size={15} />
                                </button>
                              </div>
                            </td>
                          </tr>
                        )
                      })}
                    </tbody>
                  </table>
                </div>
              </section>
            ))
          )}
        </div>
      </div>

      <Pagination page={currentPage} totalPages={totalPages} totalItems={filtered.length} pageSize={DEFAULT_PAGE_SIZE} onPage={setPage} />

      <Modal
        open={adding}
        title="Add New Student"
        size="lg"
        onClose={() => setAdding(false)}
        footer={
          <>
            <button className="btn btn-secondary" onClick={() => setAdding(false)}>Cancel</button>
            <button className="btn btn-primary" form="add-student-form" type="submit">Save Student</button>
          </>
        }
      >
        <form id="add-student-form" onSubmit={saveStudent} noValidate>
          <div className="form-row">
            <Field label="Full Name" required error={errors.name}>
              <Input value={form.name} error={errors.name} onChange={(e) => setForm({ ...form, name: e.target.value })} placeholder="e.g. Rahul Menon" />
            </Field>
            <Field label="Student ID" required error={errors.studentId}>
              <Input value={form.studentId} error={errors.studentId} onChange={(e) => setForm({ ...form, studentId: e.target.value })} placeholder="e.g. 2025CS1102" />
            </Field>
          </div>
          <div className="form-row">
            <Field label="College Email" required error={errors.email}>
              <Input type="email" value={form.email} error={errors.email} onChange={(e) => setForm({ ...form, email: e.target.value })} placeholder="name@mitsgwl.ac.in" />
            </Field>
            <Field label="Contact" required error={errors.contact}>
              <Input value={form.contact} error={errors.contact} onChange={(e) => setForm({ ...form, contact: e.target.value })} placeholder="+91 98400 00000" />
            </Field>
          </div>
          <div className="form-row">
            <Field label="Branch" required>
              <Select value={form.branch} onChange={(e) => setForm({ ...form, branch: e.target.value })}>
                {branches.map((b) => <option key={b} value={b}>{b}</option>)}
              </Select>
            </Field>
            <Field label="Program">
              <Select value={form.program} onChange={(e) => setForm({ ...form, program: e.target.value })}>
                <option>B.Tech</option>
                <option>MBA</option>
                <option>M.Sc</option>
                <option>B.Sc</option>
              </Select>
            </Field>
          </div>
          <div className="form-row">
            <Field label="Semester">
              <Input type="number" min="1" max="12" value={form.semester} onChange={(e) => setForm({ ...form, semester: e.target.value })} />
            </Field>
            <Field label="Status">
              <Select value={form.status} onChange={(e) => setForm({ ...form, status: e.target.value })}>
                <option value="active">Active</option>
                <option value="suspended">Suspended</option>
              </Select>
            </Field>
          </div>
        </form>
      </Modal>

      <ConfirmDialog
        open={Boolean(deleting)}
        title="Remove this student?"
        confirmLabel="Remove Student"
        message={deleting ? `${deleting.name} (${deleting.studentId}) and all their loans, reservations and fine records will be deleted.` : ''}
        onClose={() => setDeleting(null)}
        onConfirm={confirmDelete}
      />
    </>
  )
}
