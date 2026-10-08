/**
 * Default configuration for the Settings pages.
 *
 * Frontend-only: these values live in the library store (persisted to
 * localStorage) so the UI feels real today. When the C HTTP backend is
 * connected, each page action will POST these values (e.g. circulation ->
 * maximum_books, fine_per_day, ...) and the database will become the source
 * of truth. No credentials or secrets belong in the frontend.
 */

export const APP_VERSION = '1.0.0'

/** Single source of truth for the college name shown across the portal. */
export const INSTITUTION_NAME = 'Madhav Institute of Science & Technology'

export const defaultSettings = {
  /* Legacy flat flags kept for compatibility with older saved states. */
  institutionName: INSTITUTION_NAME,
  loanPeriodDays: 14,
  finePerDay: 10,
  maxBooksPerStudent: 4,
  reservationHoldDays: 3,
  emailReminders: true,
  smsReminders: false,
  autoOverdueReport: true,

  /* Admin: library profile. */
  library: {
    name: INSTITUTION_NAME,
    email: 'library@mitsgwalior.in',
    phone: '+91 44 2857 0100',
    address: 'Administrative Block, Madhav Institute, Gwalior, Madhya Pradesh',
    openingTime: '08:00',
    closingTime: '21:00',
    workingDays: ['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun'],
  },

  /* Admin: circulation rules. Backend column names are noted per field. */
  circulation: {
    maxBooksPerStudent: 3 /* maximum_books */,
    borrowingPeriodDays: 14 /* borrowing_period_days */,
    finePerDay: 5 /* fine_per_day */,
    maximumFine: 200 /* maximum_fine */,
    renewalEnabled: true /* renewal_enabled */,
    maxRenewalsPerBook: 2 /* maximum_renewals */,
    reservationValidityDays: 2 /* reservation_validity_days */,
    maxActiveReservations: 2 /* maximum_reservations */,
  },

  /* Admin: notification preferences. */
  notifications: {
    activity: true,
    overdue: true,
    fine: true,
    reservation: true,
    announcement: true,
    system: true,
  },

  /* Admin: email channel. */
  email: {
    enabled: true,
    dueDate: true,
    overdue: true,
    fine: true,
    reservation: true,
    announcement: true,
  },

  /* Admin: account, language and privacy. */
  language: 'en',
  privacy: { profileVisibility: 'public', showContact: true },
  security: { sessionTimeoutMinutes: 30 },

  /* Per-user student preferences, keyed by student id. */
  studentNotifications: {},
  studentPrefs: {},
}

export const ALL_DAYS = ['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun']

export const ADMIN_NOTIFICATIONS = [
  {
    key: 'activity',
    label: 'New book & reservation activity',
    hint: 'New arrivals and recent reservation requests.',
  },
  { key: 'overdue', label: 'Overdue book alerts', hint: 'Books that have crossed their due date.' },
  { key: 'fine', label: 'Fine notifications', hint: 'Fines charged or cleared for members.' },
  { key: 'reservation', label: 'Reservation notifications', hint: 'Reservation approvals and pickups.' },
  {
    key: 'announcement',
    label: 'Important library announcements',
    hint: 'Broad notices sent to all members.',
  },
  { key: 'system', label: 'System notifications', hint: 'Platform and account updates.' },
]

export const STUDENT_NOTIFICATIONS = [
  { key: 'dueDate', label: 'Due-date reminders', hint: 'Before a borrowed book is due.' },
  { key: 'overdue', label: 'Overdue alerts', hint: 'When a book crosses its due date.' },
  { key: 'fine', label: 'Fine notifications', hint: 'When a fine is charged or paid.' },
  { key: 'reservation', label: 'Reservation alerts', hint: 'When a reserved book is ready.' },
  { key: 'announcement', label: 'Library announcements', hint: 'Notices from the library.' },
  { key: 'system', label: 'System notifications', hint: 'Account and platform updates.' },
]

export const PRIVACY_OPTIONS = [
  { value: 'public', label: 'Everyone in the library network' },
  { value: 'staff', label: 'Library staff & faculty only' },
  { value: 'private', label: 'Only me' },
]