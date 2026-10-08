/**
 * Academic branches / departments of the college.
 *
 * The college has a single central library, so a book is not shelved in a
 * physical "branch" - instead every title is tagged with the academic branch
 * or subject it belongs to. In the backend this will become the `branches`
 * table.
 */
export const mockBranches = [
  { id: 'cs', name: 'Computer Science & Engineering' },
  { id: 'it', name: 'Information Technology' },
  { id: 'ec', name: 'Electronics & Communication Engineering' },
  { id: 'ee', name: 'Electrical Engineering' },
  { id: 'me', name: 'Mechanical Engineering' },
  { id: 'ce', name: 'Civil Engineering' },
  { id: 'mgmt', name: 'Management Studies' },
  { id: 'sc', name: 'General Science & Humanities' },
]

export default mockBranches