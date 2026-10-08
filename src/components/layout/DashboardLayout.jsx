import { Suspense, useEffect, useRef, useState } from 'react'
import { Outlet, useLocation } from 'react-router-dom'
import Sidebar from './Sidebar.jsx'
import Topbar from './Topbar.jsx'
import PageSkeleton from '../ui/PageSkeleton.jsx'
import LoadingState from '../ui/LoadingState.jsx'
import BackToTop from '../ui/BackToTop.jsx'
import CommandPalette from './CommandPalette.jsx'
import NotificationPanel from './NotificationPanel.jsx'
import AnnouncementPanel from './AnnouncementPanel.jsx'
import AnnouncementBanner from './AnnouncementBanner.jsx'
import { useLibrary } from '../../store/LibraryContext.jsx'
import { useAuth } from '../../store/AuthContext.jsx'
import { PageTitleContext } from '../../store/PageTitleContext.jsx'
import { notificationsOf } from '../../utils/selectors.js'
import { enhanceTables } from '../../utils/tableCards.js'

/**
 * Shared frame for both dashboards: Sidebar + Topbar + routed content.
 * The navigation sections come from StudentLayout / AdminLayout.
 * Pages can still set the browser tab title with usePageTitle('Page Name').
 */
const COLLAPSED_KEY = 'lms.sidebar.collapsed'

export default function DashboardLayout({
  sections,
  notificationsPath,
  profilePath,
  brand,
  brandSub,
}) {
  const [sidebarOpen, setSidebarOpen] = useState(false)
  const [notifOpen, setNotifOpen] = useState(false)
  const [annOpen, setAnnOpen] = useState(false)
  const [collapsed, setCollapsed] = useState(() => {
    try {
      return localStorage.getItem(COLLAPSED_KEY) === '1'
    } catch {
      return false
    }
  })
  const [, setPageMeta] = useState(null)
  const contentRef = useRef(null)
  const location = useLocation()
  const { state, booting, syncing } = useLibrary()
  const { user } = useAuth()

  // Mobile card labels for data tables (see utils/tableCards.js).
  useEffect(() => enhanceTables(contentRef.current), [booting])

  function toggleCollapsed() {
    setCollapsed((current) => {
      const next = !current
      try {
        localStorage.setItem(COLLAPSED_KEY, next ? '1' : '0')
      } catch {
        /* storage unavailable - preference applies for this session */
      }
      return next
    })
  }

  // Lock page scroll while the mobile navigation drawer is open.
  useEffect(() => {
    document.body.style.overflow = sidebarOpen ? 'hidden' : ''
    return () => {
      document.body.style.overflow = ''
    }
  }, [sidebarOpen])

  const unread = notificationsOf(state, user?.id).filter((n) => !n.read).length

  if (booting) {
    return (
      <div className="app-shell">
        <div className="main-area" style={{ marginLeft: 0 }}>
          <LoadingState label="Opening the library portal..." />
        </div>
      </div>
    )
  }

  return (
    <div className={`app-shell ${collapsed ? 'sidebar-collapsed' : ''}`}>
      <Sidebar
        sections={sections}
        open={sidebarOpen}
        onClose={() => setSidebarOpen(false)}
        brand={brand}
        brandSub={brandSub}
        collapsed={collapsed}
        onToggleCollapse={toggleCollapsed}
      />
      <div className="main-area">
        <Topbar
          onMenuClick={() => setSidebarOpen(true)}
          onBellClick={() => setNotifOpen(true)}
          onAnnounceClick={user?.role === 'admin' ? () => setAnnOpen(true) : undefined}
          unread={unread}
          syncing={syncing}
          notificationsPath={notificationsPath}
          profilePath={profilePath}
        />
        <main className="content" ref={contentRef}>
          <AnnouncementBanner />
          <PageTitleContext.Provider value={setPageMeta}>
            {/* Keyed wrapper = a fresh fade/slide on every navigation;
                the Suspense fallback (skeleton) crossfades into the page. */}
            <div className="route-view" key={location.pathname}>
              <Suspense fallback={<PageSkeleton />}>
                <Outlet />
              </Suspense>
            </div>
          </PageTitleContext.Provider>
        </main>
        <BackToTop />
        <CommandPalette sections={sections} notificationsPath={notificationsPath} />
        <NotificationPanel
          open={notifOpen}
          onClose={() => setNotifOpen(false)}
          notificationsPath={notificationsPath}
          userId={user?.id}
        />
        {user?.role === 'admin' && (
          <AnnouncementPanel
            open={annOpen}
            onClose={() => setAnnOpen(false)}
            notificationsPath="/admin/notifications"
          />
        )}
      </div>
    </div>
  )
}
