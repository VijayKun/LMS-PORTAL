import { createContext, useContext, useEffect, useReducer, useState } from 'react'
import { createInitialData } from '../data/index.js'
import { INSTITUTION_NAME } from '../data/settings.js'
import { LOAN_PERIOD_DAYS, FINE_PER_DAY } from '../data/constants.js'
import { daysBetween, todayISO, addDays } from '../utils/format.js'

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
const DATA_VERSION = 2

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
      library: { ...state.settings?.library, name: INSTITUTION_NAME },
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

  // Short simulated "fetch" so the app shows a real loading state on start.
  useEffect(() => {
    const timer = setTimeout(() => setBooting(false), 450)
    return () => clearTimeout(timer)
  }, [])

  useEffect(() => {
    try {
      localStorage.setItem(STORAGE_KEY, JSON.stringify(state))
    } catch {
      /* storage full / private mode - the app still works in memory */
    }
  }, [state])

  /**
   * Optimistic action tracking: the reducer has already committed the change
   * (the UI updates instantly), and this keeps a "syncing" counter alive
   * while the mock backend call is in flight. The topbar shows a subtle
   * pill while the counter is > 0. When a real backend arrives, failures
   * can be rolled back here without touching any page.
   */
  const track = (promise) => {
    setSyncing((count) => count + 1)
    return Promise.resolve(promise).finally(() => setSyncing((count) => count - 1))
  }

  /** Everything a page can do with the library data. */
  const actions = {
    borrowBook: (studentId, bookId) => dispatch({ type: 'borrow', studentId, bookId }),
    returnBook: (txId) => dispatch({ type: 'return', txId }),
    reserveBook: (studentId, bookId) => dispatch({ type: 'reserve', studentId, bookId }),
    cancelReservation: (reservationId) => dispatch({ type: 'cancelReservation', reservationId }),
    setReservationStatus: (reservationId, status) =>
      dispatch({ type: 'setReservationStatus', reservationId, status }),
    markNotificationRead: (id) => dispatch({ type: 'markNotificationRead', id }),
    markAllNotificationsRead: (userId) =>
      dispatch({ type: 'markAllNotificationsRead', userId }),
    announce: (title, message) => dispatch({ type: 'announce', title, message }),
    addBook: (book) => dispatch({ type: 'addBook', book }),
    updateBook: (book) => dispatch({ type: 'updateBook', book }),
    deleteBook: (bookId) => dispatch({ type: 'deleteBook', bookId }),
    addStudent: (student) => dispatch({ type: 'addStudent', student }),
    updateStudent: (student) => dispatch({ type: 'updateStudent', student }),
    deleteStudent: (studentId) => dispatch({ type: 'deleteStudent', studentId }),
    addBranch: (branch) => dispatch({ type: 'addBranch', branch }),
    updateBranch: (branch) => dispatch({ type: 'updateBranch', branch }),
    deleteBranch: (branchId) => dispatch({ type: 'deleteBranch', branchId }),
    payFine: (fineId) => dispatch({ type: 'payFine', fineId }),
    updateSettings: (settings) => dispatch({ type: 'updateSettings', settings }),
    resetData: () => dispatch({ type: 'resetData' }),
  }

  return (
    <LibraryContext.Provider value={{ state, booting, syncing, track, ...actions }}>
      {children}
    </LibraryContext.Provider>
  )
}

export function useLibrary() {
  const ctx = useContext(LibraryContext)
  if (!ctx) throw new Error('useLibrary must be used inside <LibraryProvider>')
  return ctx
}
