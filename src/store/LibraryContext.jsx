import {
  createContext,
  useCallback,
  useContext,
  useEffect,
  useReducer,
  useState,
} from 'react'
import { createInitialData } from '../data/index.js'
import { INSTITUTION_NAME } from '../data/settings.js'
import { LOAN_PERIOD_DAYS, FINE_PER_DAY } from '../data/constants.js'
import { daysBetween, todayISO, addDays } from '../utils/format.js'
import { useAuthOptional } from './AuthContext.jsx'
import { useToastOptional } from './ToastContext.jsx'
import * as backend from '../services/backend.js'

/**
 * ============================================================
 *  FRONTEND STATE STORE (mock data + localStorage)
 * ============================================================
 *
 * Every feature action of the system lives here:
 *   borrowBook, returnBook, reserveBook, cancelReservation,
 *   addBook, updateBook, deleteBook, addStudent, ...
 *
 * Today they only touch local state. When the backend arrives, each action
 * will call services/api.js first and dispatch the server's response -
 * the pages themselves will not change.
 *
 * Persisted under localStorage['lms.data.v1']. Use the "Reset demo data"
 * button on the Settings page to restore the original mock dataset.
 * ============================================================
 */
const LibraryContext = createContext(null)
const STORAGE_KEY = 'lms.data.v1'

/* Bump DATA_VERSION whenever saved mock data needs reshaping; existing
   browsers are migrated once inside migrateState() before first render. */
const DATA_VERSION_KEY = 'lms.data.version'
const DATA_VERSION = 4

const ACADEMIC_BRANCHES = [
  { id: 'cs', name: 'Computer Science & Engineering' },
  { id: 'it', name: 'Information Technology' },
  { id: 'ec', name: 'Electronics & Communication Engineering' },
  { id: 'ee', name: 'Electrical Engineering' },
  { id: 'me', name: 'Mechanical Engineering' },
  { id: 'ce', name: 'Civil Engineering' },
  { id: 'mgmt', name: 'Management Studies' },
  { id: 'sc', name: 'General Science & Humanities' },
]

/* Books before the academic-branch migration lived in physical library
   branches - remap each known book to the branch its subject belongs to. */
const LEGACY_BOOK_BRANCH = {
  bk1: 'cs', bk2: 'cs', bk3: 'cs', bk4: 'cs', bk5: 'mgmt', bk6: 'mgmt', bk7: 'cs',
  bk8: 'it', bk9: 'it', bk10: 'cs', bk11: 'cs', bk12: 'cs', bk13: 'cs', bk14: 'it',
  bk15: 'cs', bk16: 'ee', bk17: 'ee', bk18: 'ec', bk19: 'me', bk20: 'me',
  bk21: 'mgmt', bk22: 'mgmt', bk23: 'mgmt', bk24: 'mgmt', bk25: 'sc', bk26: 'sc',
  bk27: 'mgmt', bk28: 'ce',
}
const LEGACY_LIBRARY_BRANCH_FALLBACK = { main: 'cs', engineering: 'ee', management: 'mgmt' }

function isLegacyBranchList(branches) {
  return Array.isArray(branches) && branches.some((b) => LEGACY_LIBRARY_BRANCH_FALLBACK[b?.id])
}

function migrateState(state) {
  try {
    if (localStorage.getItem(DATA_VERSION_KEY) === `v${DATA_VERSION}`) return state
  } catch {
    /* storage unavailable - still apply the corrections below */
  }
  if (!state || typeof state !== 'object') return state

  const next = {
    ...state,
    settings: {
      ...state.settings,
      institutionName: INSTITUTION_NAME,
      finePerDay: FINE_PER_DAY,
      library: { ...state.settings?.library, name: INSTITUTION_NAME },
      circulation: { ...state.settings?.circulation, finePerDay: FINE_PER_DAY },
    },
  }

  /* Reshape old "library branches" into academic branches/subjects. */
  if (isLegacyBranchList(next.branches)) {
    next.branches = structuredClone(ACADEMIC_BRANCHES)
    next.books = Array.isArray(next.books)
      ? next.books.map((book) => {
          const legacy = LEGACY_BOOK_BRANCH[book?.id]
          const branch = legacy || LEGACY_LIBRARY_BRANCH_FALLBACK[book?.branchId]
          return branch ? { ...book, branchId: branch } : book
        })
      : next.books
  }

  if (Array.isArray(next.fines)) {
    next.fines = next.fines.map((fine) => {
      const overdueDays = Number(fine?.overdueDays) || 0
      return { ...fine, amount: overdueDays * FINE_PER_DAY }
    })
  }

  if (Array.isArray(next.notifications)) {
    next.notifications = next.notifications.map((notification) =>
      notification?.id === 'nt3'
        ? {
            ...notification,
            title: `Fine of ₹${FINE_PER_DAY * 5} pending`,
            message: `A fine of ₹${FINE_PER_DAY * 5} was recorded for "Don't Make Me Think" (5 days overdue). Please clear it at the library counter.`,
          }
        : notification,
    )
  }

  try {
    localStorage.setItem(DATA_VERSION_KEY, `v${DATA_VERSION}`)
  } catch {
    /* storage unavailable - the in-memory state is still migrated */
  }
  return next
}

let uidCounter = 0
const uid = (prefix) => `${prefix}${Date.now().toString(36)}${(uidCounter++).toString(36)}`

function loadState() {
  try {
    const saved = localStorage.getItem(STORAGE_KEY)
    if (saved) return migrateState(JSON.parse(saved))
  } catch {
    /* corrupted storage - fall through to fresh mock data */
  }
  return createInitialData()
}

function activeLoanCount(transactions, bookId) {
  return transactions.filter((t) => t.bookId === bookId && t.status === 'issued').length
}

/** Keeps book.availableCopies in sync with the transaction list. */
function syncAvailability(books, transactions) {
  return books.map((book) => ({
    ...book,
    availableCopies: Math.max(0, book.totalCopies - activeLoanCount(transactions, book.id)),
  }))
}

function reducer(state, action) {
  switch (action.type) {
    /* Server data replaces the local demo data after a successful fetch. */
    case 'hydrate':
      return { ...state, ...action.data, _remote: true }

    /* ------------------ student actions ------------------ */
    case 'borrow': {
      const book = state.books.find((b) => b.id === action.bookId)
      if (!book) return state
      const transactions = [
        ...state.transactions,
        {
          id: uid('tx'),
          studentId: action.studentId,
          bookId: action.bookId,
          issueDate: todayISO(),
          dueDate: addDays(todayISO(), LOAN_PERIOD_DAYS),
          returnDate: null,
          fine: 0,
          status: 'issued',
        },
      ]
      // A waiting/ready reservation for this book is satisfied by the borrow.
      const reservations = state.reservations.map((r) =>
        r.bookId === action.bookId &&
        r.studentId === action.studentId &&
        (r.status === 'waiting' || r.status === 'ready')
          ? { ...r, status: 'fulfilled' }
          : r,
      )
      return {
        ...state,
        transactions,
        reservations,
        books: syncAvailability(state.books, transactions),
      }
    }

    case 'return': {
      const tx = state.transactions.find((t) => t.id === action.txId)
      if (!tx || tx.status === 'returned') return state
      const overdueDays = Math.max(0, daysBetween(tx.dueDate, todayISO()))
      const fineAmount = overdueDays * FINE_PER_DAY
      const transactions = state.transactions.map((t) =>
        t.id === action.txId
          ? { ...t, status: 'returned', returnDate: todayISO(), fine: fineAmount }
          : t,
      )
      const fines = fineAmount
        ? [
            ...state.fines,
            {
              id: uid('fn'),
              studentId: tx.studentId,
              bookId: tx.bookId,
              dueDate: tx.dueDate,
              returnDate: todayISO(),
              overdueDays,
              amount: fineAmount,
              status: 'pending',
              paidDate: null,
            },
          ]
        : state.fines
      return { ...state, transactions, fines, books: syncAvailability(state.books, transactions) }
    }

    case 'reserve': {
      const waiting = state.reservations.filter(
        (r) => r.bookId === action.bookId && r.status !== 'expired' && r.status !== 'fulfilled',
      )
      const alreadyReserved = waiting.some(
        (r) => r.studentId === action.studentId && r.bookId === action.bookId,
      )
      if (alreadyReserved) return state
      return {
        ...state,
        reservations: [
          ...state.reservations,
          {
            id: uid('rs'),
            studentId: action.studentId,
            bookId: action.bookId,
            reservationDate: todayISO(),
            queuePosition: waiting.length + 1,
            status: 'waiting',
            expectedAvailability: addDays(todayISO(), 7),
            pickupDeadline: null,
          },
        ],
      }
    }

    case 'cancelReservation':
      return {
        ...state,
        reservations: state.reservations.filter((r) => r.id !== action.reservationId),
      }

    case 'setReservationStatus':
      return {
        ...state,
        reservations: state.reservations.map((r) =>
          r.id === action.reservationId
            ? {
                ...r,
                status: action.status,
                pickupDeadline:
                  action.status === 'ready' ? addDays(todayISO(), 3) : r.pickupDeadline,
              }
            : r,
        ),
      }

    /* ------------------ notification actions ------------------ */
    case 'markNotificationRead':
      return {
        ...state,
        notifications: state.notifications.map((n) =>
          n.id === action.id ? { ...n, read: true } : n,
        ),
      }

    case 'markAllNotificationsRead':
      return {
        ...state,
        notifications: state.notifications.map((n) =>
          n.userId === action.userId || n.userId === 'all' ? { ...n, read: true } : n,
        ),
      }

    case 'announce':
      return {
        ...state,
        notifications: [
          {
            id: uid('nt'),
            type: 'announcement',
            title: action.title,
            message: action.message,
            time: new Date().toISOString().slice(0, 19),
            read: false,
            userId: 'all',
          },
          ...state.notifications,
        ],
      }

    /* ------------------ book management ------------------ */
    case 'addBook':
      return { ...state, books: [{ ...action.book, id: uid('bk') }, ...state.books] }

    case 'updateBook':
      return {
        ...state,
        books: state.books.map((b) => (b.id === action.book.id ? { ...b, ...action.book } : b)),
      }

    case 'deleteBook':
      return {
        ...state,
        books: state.books.filter((b) => b.id !== action.bookId),
        transactions: state.transactions.filter((t) => t.bookId !== action.bookId),
        reservations: state.reservations.filter((r) => r.bookId !== action.bookId),
        fines: state.fines.filter((f) => f.bookId !== action.bookId),
      }

    /* ------------------ student management ------------------ */
    case 'addStudent':
      return { ...state, students: [{ ...action.student, id: uid('st') }, ...state.students] }

    case 'updateStudent':
      return {
        ...state,
        students: state.students.map((s) =>
          s.id === action.student.id ? { ...s, ...action.student } : s,
        ),
      }

    case 'deleteStudent':
      return {
        ...state,
        students: state.students.filter((s) => s.id !== action.studentId),
        transactions: state.transactions.filter((t) => t.studentId !== action.studentId),
        reservations: state.reservations.filter((r) => r.studentId !== action.studentId),
        fines: state.fines.filter((f) => f.studentId !== action.studentId),
      }

    /* ------------------ branch management ------------------ */
    case 'addBranch':
      return { ...state, branches: [...state.branches, { ...action.branch, id: uid('br') }] }

    case 'updateBranch':
      return {
        ...state,
        branches: state.branches.map((b) =>
          b.id === action.branch.id ? { ...b, ...action.branch } : b,
        ),
      }

    case 'deleteBranch': {
      const fallback = state.branches.find((b) => b.id !== action.branchId)
      return {
        ...state,
        branches: state.branches.filter((b) => b.id !== action.branchId),
        books: state.books.map((book) =>
          book.branchId === action.branchId ? { ...book, branchId: fallback?.id ?? null } : book,
        ),
      }
    }

    /* ------------------ fines ------------------ */
    case 'payFine':
      return {
        ...state,
        fines: state.fines.map((f) =>
          f.id === action.fineId
            ? { ...f, status: 'paid', paidDate: todayISO() }
            : f,
        ),
      }

    /* ------------------ settings ------------------ */
    case 'updateSettings':
      return { ...state, settings: { ...state.settings, ...action.settings } }

    case 'resetData':
      return createInitialData()

    default:
      return state
  }
}

export function LibraryProvider({ children }) {
  const [state, dispatch] = useReducer(reducer, undefined, loadState)
  const [booting, setBooting] = useState(true)
  const [syncing, setSyncing] = useState(0)

  const auth = useAuthOptional()
  const toastCtx = useToastOptional()
  const user = auth?.user ?? null
  const backendAvailable = auth?.backendAvailable ?? null
  const toast = toastCtx?.toast

  const remote = Boolean(state._remote)

  const track = useCallback((promise) => {
    setSyncing((count) => count + 1)
    return Promise.resolve(promise).finally(() => setSyncing((count) => count - 1))
  }, [])

  /** Re-fetch the signed-in user's workspace and replace the local snapshot. */
  const refreshFromBackend = useCallback(async () => {
    if (!user || !backendAvailable) return false
    try {
      const data = await backend.fetchWorkspace(user)
      dispatch({ type: 'hydrate', data })
      return true
    } catch (err) {
      if (err.status === 401) {
        toast?.('Your session expired. Please sign in again.', 'error')
      } else {
        toast?.(err.message || 'Could not load library data.', 'error')
      }
      return false
    }
  }, [user, backendAvailable, toast])

  // Load the real workspace after sign-in; keep the offline demo if the API is down.
  useEffect(() => {
    let cancelled = false
    if (!user) {
      const timer = setTimeout(() => {
        if (!cancelled) setBooting(false)
      }, 450)
      return () => {
        cancelled = true
        clearTimeout(timer)
      }
    }
    if (backendAvailable === null) return undefined
    if (!backendAvailable) {
      setBooting(false)
      return undefined
    }

    setSyncing((count) => count + 1)
    refreshFromBackend().finally(() => {
      if (!cancelled) {
        setBooting(false)
        setSyncing((count) => count - 1)
      }
    })
    return () => {
      cancelled = true
    }
  }, [user, backendAvailable, refreshFromBackend])

  // Persist only the offline dataset; server data must not leak between users.
  useEffect(() => {
    if (state._remote) return
    try {
      localStorage.setItem(STORAGE_KEY, JSON.stringify(state))
    } catch {
      /* storage full / private mode - the app still works in memory */
    }
  }, [state])

  // Drop the previous account's server data as soon as they sign out.
  useEffect(() => {
    if (!user && state._remote) dispatch({ type: 'resetData' })
  }, [user, state._remote])

  /**
   * Remote actions call the API and re-fetch the workspace. Offline/demo mode
   * keeps the original local reducer so the frontend is still demo-able.
   */
  const runRemote = (remoteCall, localAction, failureMessage) => {
    if (!remote) {
      dispatch(localAction)
      return
    }
    track(
      (async () => {
        try {
          await remoteCall()
          await refreshFromBackend()
        } catch (err) {
          toast?.(err.message || failureMessage, 'error')
        }
      })(),
    )
  }

  /** Everything a page can do with the library data. */
  const actions = {
    borrowBook: (studentId, bookId) =>
      runRemote(
        () => backend.issueLoan(studentId, bookId),
        { type: 'borrow', studentId, bookId },
        'Could not issue this book.',
      ),
    returnBook: (txId) =>
      runRemote(
        () => backend.returnLoan(txId),
        { type: 'return', txId },
        'Could not return this book.',
      ),
    reserveBook: (studentId, bookId) =>
      runRemote(
        () => backend.createReservation(studentId, bookId),
        { type: 'reserve', studentId, bookId },
        'Could not create this reservation.',
      ),
    cancelReservation: (reservationId) =>
      runRemote(
        () => backend.cancelReservation(reservationId),
        { type: 'cancelReservation', reservationId },
        'Could not cancel this reservation.',
      ),
    setReservationStatus: (reservationId, status) =>
      runRemote(
        () => backend.setReservationStatus(reservationId, status),
        { type: 'setReservationStatus', reservationId, status },
        'Could not update this reservation.',
      ),
    markNotificationRead: (id) =>
      runRemote(
        () => backend.markNotificationRead(id),
        { type: 'markNotificationRead', id },
        'Could not update this notification.',
      ),
    markAllNotificationsRead: (userId) =>
      runRemote(
        () => backend.markAllNotificationsRead(),
        { type: 'markAllNotificationsRead', userId },
        'Could not mark notifications as read.',
      ),
    announce: (title, message) =>
      runRemote(
        () => backend.createNotification({ type: 'announcement', title, message, userId: 'all' }),
        { type: 'announce', title, message },
        'Could not publish the announcement.',
      ),
    addBook: (book) =>
      runRemote(
        () => backend.createBook(book),
        { type: 'addBook', book },
        'Could not add this book.',
      ),
    updateBook: (book) =>
      runRemote(
        () => backend.updateBook(book),
        { type: 'updateBook', book },
        'Could not update this book.',
      ),
    deleteBook: (bookId) =>
      runRemote(
        () => backend.deleteBook(bookId),
        { type: 'deleteBook', bookId },
        'Could not delete this book.',
      ),
    addStudent: (student) =>
      runRemote(
        () => backend.createStudent(student),
        { type: 'addStudent', student },
        'Could not add this student.',
      ),
    updateStudent: (student) =>
      runRemote(
        () => backend.updateStudent(student),
        { type: 'updateStudent', student },
        'Could not update this student.',
      ),
    deleteStudent: (studentId) =>
      runRemote(
        () => backend.deleteStudent(studentId),
        { type: 'deleteStudent', studentId },
        'Could not delete this student.',
      ),
    addBranch: (branch) =>
      runRemote(
        () => backend.createBranch(branch),
        { type: 'addBranch', branch },
        'Could not add this branch.',
      ),
    updateBranch: (branch) =>
      runRemote(
        () => backend.updateBranch(branch),
        { type: 'updateBranch', branch },
        'Could not update this branch.',
      ),
    deleteBranch: (branchId) =>
      runRemote(
        () => backend.deleteBranch(branchId),
        { type: 'deleteBranch', branchId },
        'Could not delete this branch.',
      ),
    payFine: (fineId) =>
      runRemote(
        () => backend.payFine(fineId),
        { type: 'payFine', fineId },
        'Could not record this payment.',
      ),
    updateSettings: (settings) => dispatch({ type: 'updateSettings', settings }),
    resetData: () => {
      if (remote) {
        toast?.('Demo reset is only available while the backend is offline.', 'info')
        return
      }
      dispatch({ type: 'resetData' })
    },
    refreshFromBackend,
  }

  return (
    <LibraryContext.Provider
      value={{ state, booting, syncing, track, remote, ...actions }}
    >
      {children}
    </LibraryContext.Provider>
  )
}

export function useLibrary() {
  const ctx = useContext(LibraryContext)
  if (!ctx) throw new Error('useLibrary must be used inside <LibraryProvider>')
  return ctx
}
