import { lazy, Suspense } from 'react'
import { Routes, Route, Navigate } from 'react-router-dom'
import { AuthProvider, useAuth } from './store/AuthContext.jsx'
import { LibraryProvider } from './store/LibraryContext.jsx'
import { ToastProvider } from './store/ToastContext.jsx'
import { ThemeProvider } from './store/ThemeContext.jsx'
import ErrorBoundary from './components/ErrorBoundary.jsx'
import PageSkeleton from './components/ui/PageSkeleton.jsx'
import StudentLayout from './components/layout/StudentLayout.jsx'
import AdminLayout from './components/layout/AdminLayout.jsx'

// Every page is code-split: only the chunks needed for the visited route load.
const LoginPage = lazy(() => import('./pages/Login.jsx'))
const NotFoundPage = lazy(() => import('./pages/NotFound.jsx'))

const StudentDashboard = lazy(() => import('./pages/student/StudentDashboard.jsx'))
const Catalog = lazy(() => import('./pages/student/Catalog.jsx'))
const BookDetails = lazy(() => import('./pages/student/BookDetails.jsx'))
const MyBooks = lazy(() => import('./pages/student/MyBooks.jsx'))
const StudentReservations = lazy(() => import('./pages/student/StudentReservations.jsx'))
const BorrowingHistory = lazy(() => import('./pages/student/BorrowingHistory.jsx'))
const StudentFines = lazy(() => import('./pages/student/StudentFines.jsx'))
const StudentNotifications = lazy(() => import('./pages/student/StudentNotifications.jsx'))
const StudentProfile = lazy(() => import('./pages/student/StudentProfile.jsx'))
const StudentSettings = lazy(() => import('./pages/student/StudentSettings.jsx'))

const AdminDashboard = lazy(() => import('./pages/admin/AdminDashboard.jsx'))
const BookManagement = lazy(() => import('./pages/admin/BookManagement.jsx'))
const BookForm = lazy(() => import('./pages/admin/BookForm.jsx'))
const StudentManagement = lazy(() => import('./pages/admin/StudentManagement.jsx'))
const StudentDetails = lazy(() => import('./pages/admin/StudentDetails.jsx'))
const Transactions = lazy(() => import('./pages/admin/Transactions.jsx'))
const AdminReservations = lazy(() => import('./pages/admin/AdminReservations.jsx'))
const AdminFines = lazy(() => import('./pages/admin/AdminFines.jsx'))
const AdminNotifications = lazy(() => import('./pages/admin/AdminNotifications.jsx'))
const Analytics = lazy(() => import('./pages/admin/Analytics.jsx'))
const Reports = lazy(() => import('./pages/admin/Reports.jsx'))
const Settings = lazy(() => import('./pages/admin/Settings.jsx'))
const BulkImport = lazy(() => import('./pages/admin/BulkImport.jsx'))

/**
 * Route guard:
 * - not logged in  -> send to login screen
 * - wrong role     -> send to that role's own dashboard
 */
function RequireRole({ role, children }) {
  const { user } = useAuth()

  if (!user) return <Navigate to="/login" replace />
  if (user.role !== role) {
    return <Navigate to={user.role === 'admin' ? '/admin' : '/student'} replace />
  }
  return children
}

export default function App() {
  return (
    <ThemeProvider>
      <ToastProvider>
        <AuthProvider>
          <LibraryProvider>
            <ErrorBoundary>
              <Suspense fallback={<PageSkeleton />}>
                <Routes>
                  <Route path="/login" element={<LoginPage />} />
                  <Route path="/" element={<Navigate to="/student" replace />} />

                  {/* ---------------- STUDENT PANEL ---------------- */}
                  <Route
                    path="/student"
                    element={
                      <RequireRole role="student">
                        <StudentLayout />
                      </RequireRole>
                    }
                  >
                    <Route index element={<StudentDashboard />} />
                    <Route path="books" element={<Catalog />} />
                    <Route path="books/:id" element={<BookDetails />} />
                    <Route path="my-books" element={<MyBooks />} />
                    <Route path="reservations" element={<StudentReservations />} />
                    <Route path="history" element={<BorrowingHistory />} />
                    <Route path="fines" element={<StudentFines />} />
                    <Route path="notifications" element={<StudentNotifications />} />
                    <Route path="profile" element={<StudentProfile />} />
                    <Route path="settings" element={<StudentSettings />} />
                  </Route>

                  {/* ---------------- ADMIN PANEL ---------------- */}
                  <Route
                    path="/admin"
                    element={
                      <RequireRole role="admin">
                        <AdminLayout />
                      </RequireRole>
                    }
                  >
                    <Route index element={<AdminDashboard />} />
                    <Route path="books" element={<BookManagement />} />
                    <Route path="books/new" element={<BookForm mode="add" />} />
                    <Route path="books/:id/edit" element={<BookForm mode="edit" />} />
                    <Route path="import" element={<BulkImport />} />
                    <Route path="students" element={<StudentManagement />} />
                    <Route path="students/:id" element={<StudentDetails />} />
                    <Route path="transactions" element={<Transactions />} />
                    <Route path="reservations" element={<AdminReservations />} />
                    <Route path="fines" element={<AdminFines />} />
                    <Route path="notifications" element={<AdminNotifications />} />
                    <Route path="analytics" element={<Analytics />} />
                    <Route path="reports" element={<Reports />} />
                    <Route path="settings" element={<Settings />} />
                  </Route>

                  <Route path="*" element={<NotFoundPage />} />
                </Routes>
              </Suspense>
            </ErrorBoundary>
          </LibraryProvider>
        </AuthProvider>
      </ToastProvider>
    </ThemeProvider>
  )
}
