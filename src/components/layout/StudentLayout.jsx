import {
  LayoutDashboard,
  BookOpen,
  BookMarked,
  Bookmark,
  History,
  IndianRupee,
  Bell,
  UserRound,
  Settings,
} from 'lucide-react'
import DashboardLayout from './DashboardLayout.jsx'
import { useLibrary } from '../../store/LibraryContext.jsx'
import { useAuth } from '../../store/AuthContext.jsx'
import { notificationsOf } from '../../utils/selectors.js'
import { INSTITUTION_NAME } from '../../data/settings.js'

/** Student panel shell - builds the nav from the current unread count. */
export default function StudentLayout() {
  const { state } = useLibrary()
  const { user } = useAuth()
  const unread = notificationsOf(state, user?.id).filter((n) => !n.read).length

  const sections = [
    {
      label: 'Main',
      links: [
        { to: '/student', label: 'Dashboard', icon: LayoutDashboard, end: true },
        { to: '/student/books', label: 'Browse Books', icon: BookOpen },
        { to: '/student/my-books', label: 'My Books', icon: BookMarked },
      ],
    },
    {
      label: 'Library',
      links: [
        { to: '/student/reservations', label: 'Reservations', icon: Bookmark },
        { to: '/student/history', label: 'Borrowing History', icon: History },
        { to: '/student/fines', label: 'Fines', icon: IndianRupee },
      ],
    },
    {
      label: 'System',
      links: [
        { to: '/student/notifications', label: 'Notifications', icon: Bell, end: true, badge: unread },
        { to: '/student/profile', label: 'Profile', icon: UserRound, end: true },
        { to: '/student/settings', label: 'Settings', icon: Settings, end: true },
      ],
    },
  ]

  return (
    <DashboardLayout
      sections={sections}
      notificationsPath="/student/notifications"
      profilePath="/student/profile"
      brand="LMS"
      brandSub={INSTITUTION_NAME}
    />
  )
}
