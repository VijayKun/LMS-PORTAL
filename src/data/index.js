/**
 * Single entry point for all mock data.
 *
 * LibraryContext loads `createInitialData()` once and stores the result in
 * localStorage. When a real backend is added, replace this with fetch() calls
 * (see services/api.js) - the components will not need to change.
 */
import { mockBooks } from './books.js'
import { mockStudents, mockAdmin, DEMO_STUDENT_ID } from './students.js'
import { mockTransactions } from './transactions.js'
import { mockReservations } from './reservations.js'
import { mockFines } from './fines.js'
import { mockNotifications } from './notifications.js'
import { mockBranches } from './branches.js'
import { defaultSettings } from './settings.js'
import * as analytics from './analytics.js'

export function createInitialData() {
  return {
    books: structuredClone(mockBooks),
    students: structuredClone(mockStudents),
    transactions: structuredClone(mockTransactions),
    reservations: structuredClone(mockReservations),
    fines: structuredClone(mockFines),
    notifications: structuredClone(mockNotifications),
    branches: structuredClone(mockBranches),
    admin: structuredClone(mockAdmin),
    demoStudentId: DEMO_STUDENT_ID,
    analytics,
    settings: structuredClone(defaultSettings),
  }
}

export {
  mockBooks,
  mockStudents,
  mockTransactions,
  mockReservations,
  mockFines,
  mockNotifications,
  mockBranches,
}
