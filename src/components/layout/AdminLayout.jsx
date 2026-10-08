import {
  LayoutDashboard,
  Library,
  Users,
  ArrowLeftRight,
  Upload,
  Bookmark,
  IndianRupee,
  Bell,
  FileText,
  BarChart3,
  Settings,
} from 'lucide-react'
import DashboardLayout from './DashboardLayout.jsx'
import { useLibrary } from '../../store/LibraryContext.jsx'
import { useAuth } from '../../store/AuthContext.jsx'
import { notificationsOf } from '../../utils/selectors.js'
import { INSTITUTION_NAME } from '../../data/settings.js'

/** Admin / librarian panel shell. */
export default function AdminLayout() {
  const { state } = useLibrary()
  const { user } = useAuth()
  const unread = notificationsOf(state, user?.id).filter((n) => !n.read).length

  const sections = [
    {
      label: 'Main',
      links: [
        { to: '/admin', label: 'Dashboard', icon: LayoutDashboard, end: true },
        { to: '/admin/books', label: 'Inventory', icon: Library },
        { to: '/admin/students', label: 'Patrons', icon: Users },
      ],
    },
    {
      label: 'Library',
      links: [
        { to: '/admin/transactions', label: 'Counter Services', icon: ArrowLeftRight },
        { to: '/admin/import', label: 'Bulk Import', icon: Upload, end: true },
        { to: '/admin/reservations', label: 'Reservations', icon: Bookmark },
        { to: '/admin/fines', label: 'Fines', icon: IndianRupee },
        { to: '/admin/notifications', label: 'Notifications', icon: Bell, end: true, badge: unread },
      ],
    },
    {
      label: 'System',
      links: [
        { to: '/admin/reports', label: 'Reports', icon: FileText, end: true },
        { to: '/admin/analytics', label: 'Analytics', icon: BarChart3, end: true },
        { to: '/admin/settings', label: 'Settings', icon: Settings, end: true },
      ],
    },
  ]

  return (
    <DashboardLayout
      sections={sections}
      notificationsPath="/admin/notifications"
      profilePath="/admin/settings"
      brand="LMS"
      brandSub={INSTITUTION_NAME}
    />
  )
}
