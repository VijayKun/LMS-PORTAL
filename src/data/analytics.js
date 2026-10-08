/**
 * Static chart data for the analytics dashboard.
 * Everything here is mock data - swap for API responses later.
 */

/** Books issued per month (last 8 months). */
export const monthlyBorrowing = [
  { month: 'Mar', issued: 186, returned: 174 },
  { month: 'Apr', issued: 162, returned: 170 },
  { month: 'May', issued: 118, returned: 132 },
  { month: 'Jun', issued: 94, returned: 88 },
  { month: 'Jul', issued: 143, returned: 121 },
  { month: 'Aug', issued: 231, returned: 208 },
  { month: 'Sep', issued: 268, returned: 246 },
  { month: 'Oct', issued: 97, returned: 74 },
]

/** Most borrowed titles. */
export const popularBooks = [
  { title: 'Atomic Habits', borrows: 87 },
  { title: 'Clean Code', borrows: 74 },
  { title: 'The Psychology of Money', borrows: 69 },
  { title: 'Introduction to Algorithms', borrows: 61 },
  { title: 'Wings of Fire', borrows: 55 },
  { title: 'Operating System Concepts', borrows: 48 },
  { title: 'Marketing Management', borrows: 41 },
]

/** Most active borrowers. */
export const activeStudents = [
  { name: 'Priya Menon', borrows: 18 },
  { name: 'Ishita Deshpande', borrows: 15 },
  { name: 'Shivangi Verma', borrows: 14 },
  { name: 'Karthik Reddy', borrows: 12 },
  { name: 'Fahad Khan', borrows: 11 },
]

/** Fine collection trend. */
export const fineTrend = [
  { month: 'Jun', collected: 420, pending: 90 },
  { month: 'Jul', collected: 510, pending: 120 },
  { month: 'Aug', collected: 680, pending: 70 },
  { month: 'Sep', collected: 910, pending: 150 },
  { month: 'Oct', collected: 340, pending: 90 },
]

/** Overdue books grouped by how late they are. */
export const overdueBuckets = [
  { label: '1-3 days', count: 3 },
  { label: '4-7 days', count: 2 },
  { label: '8-14 days', count: 1 },
  { label: '15+ days', count: 0 },
]

export default { monthlyBorrowing, popularBooks, activeStudents, fineTrend, overdueBuckets }
