import { useState } from 'react'
import { Link, useNavigate, useParams } from 'react-router-dom'
import { Sparkles, CheckCircle2 } from 'lucide-react'
import { useLibrary } from '../../store/LibraryContext.jsx'
import { usePageTitle } from '../../store/PageTitleContext.jsx'
import { useToast } from '../../store/ToastContext.jsx'
import { Field, Input, Select, Textarea } from '../../components/ui/Field.jsx'
import BookCover from '../../components/books/BookCover.jsx'
import EmptyState from '../../components/ui/EmptyState.jsx'
import Breadcrumbs from '../../components/ui/Breadcrumbs.jsx'
import { lookupBookByISBN } from '../../services/api.js'
import { mockCategories } from '../../data/books.js'

const BLANK = {
  title: '',
  author: '',
  isbn: '',
  category: '',
  publisher: '',
  publicationDate: '',
  description: '',
  cover: '#33658A',
  totalCopies: 1,
  branchId: 'cs',
  year: 0,
}

export default function BookForm({ mode }) {
  const isEdit = mode === 'edit'
  usePageTitle(isEdit ? 'Edit Book' : 'Add Book')
  const { state, addBook, updateBook } = useLibrary()
  const { toast } = useToast()
  const navigate = useNavigate()
  const { id } = useParams()

  const existing = isEdit ? state.books.find((b) => b.id === id) : null
  const [form, setForm] = useState(existing ? { ...existing } : { ...BLANK })
  const [errors, setErrors] = useState({})
  const [looking, setLooking] = useState(false)
  const [lookupMsg, setLookupMsg] = useState('')

  if (isEdit && !existing) {
    return (
      <div className="card">
        <EmptyState
          icon={Sparkles}
          title="Book not found"
          message="This title may have been removed from the catalog."
          action={<Link className="btn btn-primary" to="/admin/books">Back to books</Link>}
        />
      </div>
    )
  }

  function set(key, value) {
    setForm((f) => ({ ...f, [key]: value }))
    setErrors((e) => ({ ...e, [key]: undefined }))
  }

  /** Fetches title/author/publisher/etc. for the ISBN from the backend proxy. */
  async function fetchByISBN() {
    if (!/^\d{10,13}$/.test(form.isbn.replace(/-/g, ''))) {
      setErrors((e) => ({ ...e, isbn: 'Enter a valid 10 or 13 digit ISBN first.' }))
      return
    }
    setLooking(true)
    setLookupMsg('')
    try {
      const info = await lookupBookByISBN(form.isbn)
      if (info.found) {
        setForm((f) => ({
          ...f,
          title: info.title || f.title,
          author: info.author || f.author,
          category: info.category || f.category,
          publisher: info.publisher || f.publisher,
          publicationDate: info.publicationDate || f.publicationDate,
          description: info.description || f.description,
          cover: info.cover || f.cover,
        }))
        setLookupMsg(
          String(info.source).startsWith('google')
            ? 'Details and cover fetched from Google Books.'
            : 'Details fetched from the local catalog (backend offline).',
        )
      } else {
        setLookupMsg(
          info.source === 'google'
            ? 'No match found on Google Books — please fill in the details manually.'
            : 'No match found — please fill in the details manually.',
        )
      }
    } finally {
      setLooking(false)
    }
  }

  function validate() {
    const e = {}
    if (!form.title.trim()) e.title = 'Title is required.'
    if (!form.author.trim()) e.author = 'Author is required.'
    if (!/^\d{10}(-?\d)?(\d|\-)*$/.test(form.isbn.replace(/-/g, '')) || form.isbn.replace(/-/g, '').length < 10)
      e.isbn = 'ISBN must be 10 or 13 digits.'
    if (!form.category.trim()) e.category = 'Category is required.'
    if (!form.publisher.trim()) e.publisher = 'Publisher is required.'
    if (!form.publicationDate) e.publicationDate = 'Publication date is required.'
    if (!form.branchId) e.branchId = 'Select a branch.'
    const copies = Number(form.totalCopies)
    if (!Number.isInteger(copies) || copies < 1) e.totalCopies = 'Copies must be a whole number (min 1).'
    if (isEdit) {
      const issued = state.transactions.filter((t) => t.bookId === existing.id && t.status === 'issued').length
      if (copies < issued) e.totalCopies = `Cannot be lower than ${issued} copies currently issued.`
    }
    setErrors(e)
    return Object.keys(e).length === 0
  }

  function handleSubmit(e) {
    e.preventDefault()
    if (!validate()) return

    const payload = {
      ...form,
      totalCopies: Number(form.totalCopies),
      availableCopies: Number(form.totalCopies),
      year: Number(form.year) || 0,
    }

    if (isEdit) {
      updateBook({ ...existing, ...payload })
      toast('Book updated successfully.', 'success')
    } else {
      addBook(payload)
      toast('Book added to the catalog successfully.', 'success')
    }
    navigate('/admin/books')
  }

  return (
    <>
      <Breadcrumbs
        items={[
          { label: 'Books', to: '/admin/books' },
          { label: isEdit ? 'Edit book' : 'Add book' },
        ]}
      />
      <div className="page-header">
        <div>
          <h1>{isEdit ? 'Edit Book' : 'Add New Book'}</h1>
          <p className="page-sub">
            {isEdit ? 'Update the catalog record for this title.' : 'Add a single title to the library catalog.'}
          </p>
        </div>
      </div>

      <div className="grid grid-split" style={{ gridTemplateColumns: 'minmax(0, 1.6fr) minmax(0, 0.8fr)' }}>
        <form className="card card-pad" onSubmit={handleSubmit} noValidate>
          <div className="form-row">
            <Field label="Book Title" required error={errors.title} htmlFor="f-title">
              <Input id="f-title" value={form.title} error={errors.title} onChange={(e) => set('title', e.target.value)} placeholder="e.g. Clean Code" />
            </Field>
            <Field label="Author" required error={errors.author} htmlFor="f-author">
              <Input id="f-author" value={form.author} error={errors.author} onChange={(e) => set('author', e.target.value)} placeholder="e.g. Robert C. Martin" />
            </Field>
          </div>

          <div className="form-row">
            <Field label="ISBN" required error={errors.isbn} htmlFor="f-isbn" hint="10 or 13 digits.">
              <div className="row" style={{ gap: 8 }}>
                <Input id="f-isbn" value={form.isbn} error={errors.isbn} onChange={(e) => set('isbn', e.target.value)} placeholder="9780132350884" />
                <button type="button" className="btn btn-secondary nowrap" onClick={fetchByISBN} disabled={looking}>
                  <Sparkles size={14} /> {looking ? 'Fetching...' : 'Fetch'}
                </button>
              </div>
            </Field>
            <Field label="Category" required error={errors.category} htmlFor="f-category">
              <Input id="f-category" list="category-list" value={form.category} error={errors.category} onChange={(e) => set('category', e.target.value)} placeholder="Computer Science" />
              <datalist id="category-list">
                {mockCategories.map((c) => <option key={c} value={c} />)}
              </datalist>
            </Field>
          </div>

          <div className="form-row">
            <Field label="Publisher" required error={errors.publisher} htmlFor="f-publisher">
              <Input id="f-publisher" value={form.publisher} error={errors.publisher} onChange={(e) => set('publisher', e.target.value)} placeholder="Prentice Hall" />
            </Field>
            <Field label="Publication Date" required error={errors.publicationDate} htmlFor="f-date">
              <Input id="f-date" type="date" value={form.publicationDate} error={errors.publicationDate} onChange={(e) => set('publicationDate', e.target.value)} />
            </Field>
          </div>

          <Field label="Description" htmlFor="f-desc">
            <Textarea id="f-desc" value={form.description} onChange={(e) => set('description', e.target.value)} placeholder="Short summary of the book..." />
          </Field>

          <div className="form-row">
            <Field label="Total Copies" required error={errors.totalCopies} htmlFor="f-copies">
              <Input id="f-copies" type="number" min="1" value={form.totalCopies} error={errors.totalCopies} onChange={(e) => set('totalCopies', e.target.value)} />
            </Field>
            <Field label="Branch" required error={errors.branchId} htmlFor="f-branch">
              <Select id="f-branch" value={form.branchId} error={errors.branchId} onChange={(e) => set('branchId', e.target.value)}>
                {state.branches.map((b) => <option key={b.id} value={b.id}>{b.name}</option>)}
              </Select>
            </Field>
          </div>

          <Field label="Book Year" htmlFor="f-year" hint="Which student year reads this book.">
            <Select id="f-year" value={Number(form.year) || 0} onChange={(e) => set('year', Number(e.target.value))}>
              <option value={0}>All years</option>
              <option value={1}>1st year</option>
              <option value={2}>2nd year</option>
              <option value={3}>3rd year</option>
              <option value={4}>4th year</option>
            </Select>
          </Field>

          <Field label="Cover Colour" htmlFor="f-cover" hint="Used as the cover until real images are uploaded.">
            <div className="row" style={{ gap: 10 }}>
              <input id="f-cover" type="color" value={form.cover} onChange={(e) => set('cover', e.target.value)} style={{ width: 46, height: 38, padding: 3, border: '1px solid var(--border-strong)', borderRadius: 8, background: '#fff', cursor: 'pointer' }} />
              <Input value={form.cover} onChange={(e) => set('cover', e.target.value)} style={{ maxWidth: 140 }} />
            </div>
          </Field>

          <hr className="divider" />
          <div className="row" style={{ gap: 10 }}>
            <button type="submit" className="btn btn-primary">
              <CheckCircle2 size={16} /> {isEdit ? 'Save Changes' : 'Save Book'}
            </button>
            <button type="button" className="btn btn-secondary" onClick={() => navigate('/admin/books')}>
              Cancel
            </button>
          </div>
        </form>

        <aside className="card card-pad" style={{ alignSelf: 'start' }}>
          <div className="section-title mb-3">Cover preview</div>
          <BookCover book={{ ...form, author: form.author || 'Author' }} />
          <p className="text-xs text-muted mt-3">
            This is how the book will appear in the catalog grid and tables.
          </p>
          {lookupMsg && (
            <div className="demo-hint mt-3" style={{ display: 'flex', gap: 8 }}>
              <CheckCircle2 size={15} style={{ flexShrink: 0, marginTop: 2 }} />
              {lookupMsg}
            </div>
          )}
          <hr className="divider" />
          <div className="text-xs text-muted">
            The <strong>Fetch</strong> button looks the ISBN up on Google Books (via the backend)
            and fills in the title, author, publisher, category and description.
          </div>
        </aside>
      </div>
    </>
  )
}
