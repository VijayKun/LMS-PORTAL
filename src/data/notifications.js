/**
 * Notification centre mock data.
 * Shape: { id, type, title, message, time, read, userId }
 *
 * type: 'due' | 'overdue' | 'fine' | 'reservation' | 'announcement'
 * userId: a student id, 'ad1' for the librarian, or 'all' for everyone.
 */
export const mockNotifications = [
  {
    id: 'nt1',
    type: 'due',
    title: 'Due date reminder',
    message: '"The Psychology of Money" is due on 10 Oct 2026. Please return it on time to avoid a fine.',
    time: '2026-10-07T09:15:00',
    read: false,
    userId: 'st1',
  },
  {
    id: 'nt2',
    type: 'reservation',
    title: 'Reservation ready for pickup',
    message: '"Artificial Intelligence: A Modern Approach" is ready for pickup at the Engineering Library until 09 Oct.',
    time: '2026-10-06T14:30:00',
    read: false,
    userId: 'st1',
  },
  {
    id: 'nt3',
    type: 'fine',
    title: 'Fine of ₹30 pending',
    message: 'A fine of ₹30 was recorded for "Don\'t Make Me Think" (3 days overdue). Please clear it at the library counter.',
    time: '2026-10-07T08:05:00',
    read: false,
    userId: 'st1',
  },
  {
    id: 'nt4',
    type: 'due',
    title: 'Due date reminder',
    message: '"Clean Code" is due on 12 Oct 2026.',
    time: '2026-10-06T10:00:00',
    read: true,
    userId: 'st1',
  },
  {
    id: 'nt5',
    type: 'announcement',
    title: 'Library closed on 18 October',
    message: 'The Main Library will be closed on 18 Oct 2026 for annual inventory. Please plan your borrows accordingly.',
    time: '2026-10-04T16:45:00',
    read: true,
    userId: 'all',
  },
  {
    id: 'nt6',
    type: 'reservation',
    title: 'Reservation expired',
    message: 'Your reservation for "Computer Networking: A Top-Down Approach" expired on 25 Sep. You can reserve it again.',
    time: '2026-09-25T11:00:00',
    read: true,
    userId: 'st1',
  },
  {
    id: 'nt7',
    type: 'overdue',
    title: 'Overdue books need attention',
    message: '5 books are overdue across the library. Reminder SMS have been queued for the borrowers.',
    time: '2026-10-07T08:30:00',
    read: false,
    userId: 'ad1',
  },
  {
    id: 'nt8',
    type: 'announcement',
    title: 'New students registered',
    message: '24 new student accounts were imported from the college ERP this morning.',
    time: '2026-10-07T07:50:00',
    read: false,
    userId: 'ad1',
  },
  {
    id: 'nt9',
    type: 'announcement',
    title: 'Bulk import completed',
    message: 'Book import finished: 48 added, 2 failed, 1 skipped. Review the error report in Bulk Import.',
    time: '2026-10-05T12:20:00',
    read: true,
    userId: 'ad1',
  },
  {
    id: 'nt10',
    type: 'announcement',
    title: 'Mid-semester reading week',
    message: 'Loan period is extended to 21 days during reading week (12–19 Oct).',
    time: '2026-10-03T09:00:00',
    read: true,
    userId: 'all',
  },
]

export default mockNotifications
