import { useEffect, useRef, useState } from 'react'
import { Link } from 'react-router-dom'
import {
  UploadCloud,
  FileSpreadsheet,
  CheckCircle2,
  XCircle,
  AlertTriangle,
  Download,
  RotateCcw,
} from 'lucide-react'
import { usePageTitle } from '../../store/PageTitleContext.jsx'
import { useToast } from '../../store/ToastContext.jsx'
import { useLibrary } from '../../store/LibraryContext.jsx'
import Badge from '../../components/ui/Badge.jsx'
import Breadcrumbs from '../../components/ui/Breadcrumbs.jsx'
import { importBooksRequest, lookupBookByISBN } from '../../services/api.js'
import { parseCsv, toCsv, downloadTextFile } from '../../utils/csv.js'
import { buildImportRows, mergeBookInfo, normalizeSheetResult } from '../../utils/importRows.js'
import { MAX_IMPORT_ROWS } from '../../validation/schemas.js'
import readXlsxFile from 'read-excel-file/browser'

const MAX_FILE_SIZE = 5 * 1024 * 1024
const ENRICH_CONCURRENCY = 5

const STEPS = [
  { label: 'Reading file', note: 'Parsing the spreadsheet rows' },
  { label: 'Validating ISBN', note: 'Checking every ISBN format and checksum' },
  { label: 'Fetching book information', note: 'Filling missing details from Google Books' },
  { label: 'Preparing records', note: 'Matching branches, categories and copies' },
  { label: 'Import complete', note: 'Records written to the catalog' },
]

const COVER_COLORS = ['#2F4858', '#33658A', '#4527A0', '#2E7D32', '#AD1457', '#E65100', '#00695C', '#5D4037']

export default function BulkImport() {
  usePageTitle('Bulk Import')
  const { toast } = useToast()
  const { state, addBook, remote, refreshFromBackend } = useLibrary()

  const [file, setFile] = useState(null)
  const [dragging, setDragging] = useState(false)
  const [phase, setPhase] = useState('idle') // idle | running | done
  const [step, setStep] = useState(-1)
  const [rows, setRows] = useState([])
  const [enrichedCount, setEnrichedCount] = useState(0)
  const inputRef = useRef(null)
  const timers = useRef([])

  useEffect(() => () => timers.current.forEach(clearTimeout), [])

  /** Reads the chosen file for real: CSV via the built-in parser, XLSX via read-excel-file. */
  async function pickFile(selected) {
    if (!selected) return
    const name = selected.name.toLowerCase()

    if (selected.size > MAX_FILE_SIZE) {
      toast('File is larger than 5 MB. Please split it into smaller batches.', 'error')
      return
    }

    let table
    try {
      if (name.endsWith('.csv')) {
        table = parseCsv(await selected.text())
      } else if (name.endsWith('.xlsx')) {
        const result = await readXlsxFile(selected)
        const normalized = normalizeSheetResult(result)

        if (!normalized || normalized.length === 0) {
          throw new Error('Excel file is empty or could not be read')
        }
        if (!Array.isArray(normalized[0])) {
          throw new Error('Unexpected Excel row format')
        }
        table = normalized
      } else if (name.endsWith('.xls')) {
        toast('Legacy .xls files are not supported. Save the sheet as .xlsx or .csv and try again.', 'error')
        return
      } else {
        toast('Unsupported file. Please choose a .xlsx or .csv file.', 'error')
        return
      }
    } catch (e) {
      toast(e.message || 'That file could not be read. Make sure it is a valid spreadsheet.', 'error')
      return
    }

    let built
    try {
      built = buildImportRows(table, {
        existingBooks: state.books,
        branches: state.branches,
      })
    } catch (err) {
      toast(err.message || 'The file could not be parsed.', 'error')
      return
    }

    if (built.length > MAX_IMPORT_ROWS) {
      toast(`This file has ${built.length} rows. The limit is ${MAX_IMPORT_ROWS} per import — please split it.`, 'error')
      return
    }

    timers.current.forEach(clearTimeout)
    setFile(selected)
    setRows(built)
    setEnrichedCount(0)
    setPhase('idle')
    setStep(-1)
  }

  /** Look up every ready row so even two-column sheets get full metadata. */
  async function enrichRows(list) {
    const indices = list.reduce((acc, row, index) => {
      if (row.initial === 'ready') acc.push(index)
      return acc
    }, [])
    const enriched = [...list]
    let found = 0

    for (let i = 0; i < indices.length; i += ENRICH_CONCURRENCY) {
      const batch = indices.slice(i, i + ENRICH_CONCURRENCY)
      const results = await Promise.all(
        batch.map((index) => lookupBookByISBN(list[index].isbn).catch(() => null)),
      )
      batch.forEach((index, position) => {
        if (results[position]?.found) found++
        enriched[index] = mergeBookInfo(enriched[index], results[position])
      })
    }
    setEnrichedCount(found)
    return enriched
  }

  /** Import pipeline: validate, enrich, then write the catalog. */
  async function runImport() {
    setPhase('running')
    timers.current.forEach(clearTimeout)
    timers.current = []

    const wait = (ms) =>
      new Promise((resolve) => {
        timers.current.push(setTimeout(resolve, ms))
      })

    try {
      setStep(0)
      await wait(600)
      setStep(1)
      await wait(900)
      setStep(2)
      const enriched = await enrichRows(rows)
      setRows(enriched)
      await wait(400)
      setStep(3)
      await wait(700)
      setStep(4)

      const ready = enriched.filter((r) => r.initial === 'ready')
      let finalRows

      if (remote) {
        // One atomic request; the backend owns the authoritative result codes.
        const response = await importBooksRequest(ready)
        const codes = Array.isArray(response?.results) ? response.results : []
        const codeByKey = new Map(ready.map((r, i) => [r.key, codes[i] || 'failed']))
        finalRows = enriched.map((r) => {
          if (r.initial !== 'ready') return r
          const code = codeByKey.get(r.key)
          return {
            ...r,
            result: code === 'ok' ? 'imported' : code === 'duplicate' ? 'skipped' : 'failed',
          }
        })
        await refreshFromBackend()
      } else {
        // Offline demo mode: no request, apply the parsed rows locally.
        ready.forEach((r, i) => {
          addBook({
            title: r.title || '',
            author: r.author || '',
            isbn: r.isbn || '',
            category: r.category || 'General',
            publisher: r.publisher || '',
            publicationDate: r.publicationDate || '',
            description: r.description || '',
            cover: r.cover || COVER_COLORS[i % COVER_COLORS.length],
            totalCopies: r.copies || 1,
            availableCopies: r.copies || 1,
            branchId: r.branchId || null,
          })
        })
        finalRows = enriched.map((r) =>
          r.initial === 'ready' ? { ...r, result: 'imported' } : r,
        )
      }

      setRows(finalRows)

      const importedCount = finalRows.filter((r) => r.result === 'imported').length
      const failedCount = finalRows.filter((r) => r.result === 'failed').length
      const skippedCount = finalRows.filter((r) => r.result === 'skipped').length
      setPhase('done')
      toast(
        `Bulk import completed: ${importedCount} added, ${failedCount} failed, ${skippedCount} skipped.`,
        'success',
      )
    } catch (e) {
      console.error('Import error:', e)
      setPhase('idle')
      toast(e.message || 'The import could not be completed. Please try again.', 'error')
    }
  }

  function reset() {
    timers.current.forEach(clearTimeout)
    setFile(null)
    setRows([])
    setEnrichedCount(0)
    setPhase('idle')
    setStep(-1)
  }

  /** Real CSV download of every row that failed or was skipped, with the reason. */
  function downloadErrorReport() {
    const problems = rows.filter((r) => r.initial !== 'ready')
    if (problems.length === 0) {
      toast('Every row was imported - there are no errors to download.', 'info')
      return
    }
    const csv = toCsv(
      ['Row', 'Book Name', 'ISBN', 'Status', 'Reason'],
      problems.map((r) => [r.key + 1, r.title, r.isbn, r.initial, r.reason]),
    )
    downloadTextFile(`import-errors-${file?.name?.replace(/\.[^.]+$/, '') || 'report'}.csv`, csv)
    toast(`Error report downloaded (${problems.length} row(s)).`, 'success')
  }

  const imported = rows.filter((r) => r.result === 'imported').length
  const failed = rows.filter((r) => r.result === 'failed').length
  const skipped = rows.filter((r) => r.result === 'skipped').length

  // Pre-import validation summary (from the parsed file)
  const readyRows = rows.filter((r) => r.initial === 'ready').length
  const dupeRows = rows.filter((r) => r.initial === 'duplicate').length
  const badRows = rows.length - readyRows - dupeRows
  const progress = phase === 'done' ? 100 : phase === 'running' ? Math.round(((step + 1) / STEPS.length) * 100) : 0

  return (
    <>
      <Breadcrumbs
        items={[
          { label: 'Books', to: '/admin/books' },
          { label: 'Bulk import' },
        ]}
      />
      <div className="page-header no-print">
        <div>
          <h1>Import Books</h1>
          <p className="page-sub">
            Upload a spreadsheet of titles to add them to the catalog in one go.
          </p>
        </div>
        {file && (
          <div className="page-actions">
            <button className="btn btn-secondary" onClick={reset}>
              <RotateCcw size={15} /> Start over
            </button>
          </div>
        )}
      </div>

      <div className="grid grid-split" style={{ gridTemplateColumns: 'minmax(0, 1.5fr) minmax(0, 1fr)' }}>
        <div className="stack stack-md">
          {/* -------- Upload -------- */}
          <div className="card card-pad">
            {!file ? (
              <div
                className={`dropzone ${dragging ? 'dragging' : ''}`}
                onClick={() => inputRef.current?.click()}
                onDragOver={(e) => { e.preventDefault(); setDragging(true) }}
                onDragLeave={() => setDragging(false)}
                onDrop={(e) => { e.preventDefault(); setDragging(false); pickFile(e.dataTransfer.files?.[0]) }}
                role="button"
                tabIndex={0}
                onKeyDown={(e) => e.key === 'Enter' && inputRef.current?.click()}
              >
                <div className="drop-icon"><UploadCloud size={24} /></div>
                <h3>Drag &amp; drop your Excel or CSV file</h3>
                <p>or click to browse from your computer</p>
                <div className="row mt-3" style={{ justifyContent: 'center', gap: 8 }}>
                  <Badge tone="success">.xlsx</Badge>
                  <Badge tone="success">.csv</Badge>
                  <Badge tone="gray">Max 5 MB</Badge>
                </div>
                <input
                  ref={inputRef}
                  type="file"
                  accept=".csv,.xlsx"
                  hidden
                  onChange={(e) => pickFile(e.target.files?.[0])}
                />
              </div>
            ) : (
              <div className="stack stack-sm">
                <div className="row-between wrap">
                  <div className="row">
                    <div className="stat-icon green"><FileSpreadsheet size={19} /></div>
                    <div>
                      <div className="font-semibold">{file.name}</div>
                      <div className="text-xs text-muted">
                        {Math.max(1, Math.round(file.size / 1024))} KB · {rows.length} rows detected
                      </div>
                    </div>
                  </div>
                  <Badge status={phase === 'done' ? 'success' : phase === 'running' ? 'info' : 'waiting'}>
                    {phase === 'done' ? 'Imported' : phase === 'running' ? `Importing ${progress}%` : 'Ready to import'}
                  </Badge>
                </div>

                {phase === 'running' && (
                  <div className="import-progress" role="progressbar" aria-valuenow={progress} aria-valuemin="0" aria-valuemax="100">
                    <div className="import-progress-fill" style={{ width: `${progress}%` }} />
                  </div>
                )}
              </div>
            )}

            {file && phase === 'idle' && (
              <div className="mt-4 row wrap" style={{ gap: 12 }}>
                <button className="btn btn-primary" onClick={runImport}>
                  <UploadCloud size={16} /> Import {readyRows} {readyRows === 1 ? 'book' : 'books'}
                </button>
                <div className="row wrap" style={{ gap: 6 }}>
                  <span className="summary-chip ok">{readyRows} ready</span>
                  {dupeRows > 0 && <span className="summary-chip muted">{dupeRows} duplicate</span>}
                  {badRows > 0 && <span className="summary-chip bad">{badRows} need fixes</span>}
                </div>
              </div>
            )}
            {file && phase === 'idle' && (
              <p className="text-xs text-muted mt-3">
                Required columns: <strong>Book Name</strong>, <strong>ISBN</strong> · Optional: Copies, Branch, Author, Category, Publisher
              </p>
            )}
          </div>

          {/* -------- Preview -------- */}
          {rows.length > 0 && (
            <div className="card">
              <div className="card-header">
                <h3>Import Preview</h3>
                <div className="card-actions">
                  <span className="summary-chip ok">{readyRows} ready</span>
                  <span className="summary-chip muted">{dupeRows} duplicate</span>
                  <span className="summary-chip bad">{badRows} need fixes</span>
                </div>
              </div>
              <div className="card-body flush">
                <div className="table-responsive import-preview-scroll">
                  <table className="table">
                    <thead>
                      <tr>
                        <th>#</th>
                        <th>Book Name</th>
                        <th>ISBN</th>
                        <th>Branch</th>
                        <th>Status</th>
                      </tr>
                    </thead>
                    <tbody>
                      {rows.map((row, index) => {
                        let badge = <Badge tone="gray">Queued</Badge>
                        if (phase === 'running' && step >= 1) {
                          badge = row.initial === 'bad-isbn'
                            ? <Badge status="failed">Invalid ISBN</Badge>
                            : row.initial === 'missing-title'
                              ? <Badge status="failed">No title</Badge>
                              : row.initial === 'duplicate'
                                ? <Badge tone="gray">Duplicate</Badge>
                                : <Badge tone="info">Validating</Badge>
                        }
                        if (phase === 'done') {
                          badge =
                            row.result === 'imported'
                              ? <Badge status="success">Imported</Badge>
                              : row.result === 'failed'
                                ? <Badge status="failed">Failed</Badge>
                                : <Badge tone="gray">Skipped</Badge>
                        }
                        return (
                          <tr key={row.key}>
                            <td className="text-muted">{index + 1}</td>
                            <td className="strong">{row.title}</td>
                            <td className="text-sm">{row.isbn}</td>
                            <td className="text-sm">{row.branchLabel}</td>
                            <td>{badge}</td>
                          </tr>
                        )
                      })}
                    </tbody>
                  </table>
                </div>
              </div>
            </div>
          )}
        </div>

        {/* -------- Pipeline / results -------- */}
        <aside className="stack stack-md">
          <div className="card card-pad">
            <div className="section-title mb-4">Import process</div>
            <div className="import-steps">
              {STEPS.map((s, index) => {
                const done = phase === 'done' || step > index
                const active = phase === 'running' && step === index
                return (
                  <div key={s.label} className={`import-step ${done ? 'done' : ''} ${active ? 'active' : ''}`}>
                    <div className="step-dot">
                      {done ? <CheckCircle2 size={15} /> : active ? '•' : index + 1}
                    </div>
                    <div>
                      <div className="step-label">{s.label}</div>
                      <div className="step-note">{s.note}</div>
                    </div>
                  </div>
                )
              })}
            </div>
            {step >= 2 && (
              <p className="text-xs text-muted mt-3">
                Google Books supplied metadata and covers for {enrichedCount} of {readyRows} book(s).
              </p>
            )}
          </div>

          {phase === 'done' && (
            <div className="card card-pad">
              <div className="section-title mb-3">Import result</div>
              <div className="result-tiles">
                <div className="result-tile ok">
                  <span className="result-num">{imported}</span>
                  <span className="result-lbl"><CheckCircle2 size={13} /> Imported</span>
                </div>
                <div className="result-tile bad">
                  <span className="result-num">{failed}</span>
                  <span className="result-lbl"><XCircle size={13} /> Failed</span>
                </div>
                <div className="result-tile warn">
                  <span className="result-num">{skipped}</span>
                  <span className="result-lbl"><AlertTriangle size={13} /> Skipped</span>
                </div>
              </div>
              <div className="stack stack-sm mt-4">
                <button className="btn btn-secondary btn-block" onClick={downloadErrorReport}>
                  <Download size={15} /> Download error report
                </button>
                <Link className="btn btn-primary btn-block" to="/admin/books">
                  View catalog
                </Link>
              </div>
            </div>
          )}

          <div className="card card-pad">
            <div className="section-title mb-3">Expected columns</div>
            <div className="stack stack-sm text-sm">
              <div className="col-group">
                <span className="col-group-label required">Required</span>
                <div className="row wrap" style={{ gap: 6 }}>
                  <Badge tone="primary">Book Name</Badge>
                  <Badge tone="primary">ISBN</Badge>
                </div>
              </div>
              <div className="col-group">
                <span className="col-group-label">Optional</span>
                <div className="row wrap" style={{ gap: 6 }}>
                  <Badge tone="gray">Copies</Badge>
                  <Badge tone="gray">Branch</Badge>
                  <Badge tone="gray">Author</Badge>
                  <Badge tone="gray">Category</Badge>
                  <Badge tone="gray">Publisher</Badge>
                </div>
              </div>
            </div>
            <p className="text-xs text-muted mt-3">
              Header names are matched loosely (e.g. “Title”, “Qty”, “Location”).
            </p>
          </div>
        </aside>
      </div>
    </>
  )
}
