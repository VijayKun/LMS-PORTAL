import { beforeEach, describe, expect, it } from 'vitest'
import { act, render } from '@testing-library/react'
import { LibraryProvider, useLibrary } from './LibraryContext.jsx'

globalThis.IS_REACT_ACT_ENVIRONMENT = true

let store
function Probe() {
  store = useLibrary()
  return null
}

function renderStore() {
  render(
    <LibraryProvider>
      <Probe />
    </LibraryProvider>,
  )
}

beforeEach(() => {
  store = undefined
  localStorage.clear()
  renderStore()
})

describe('LibraryProvider', () => {
  it('boots with the seed dataset', () => {
    expect(store.booting).toBe(true)
    expect(store.state.books.length).toBeGreaterThan(0)
    expect(store.state.branches.length).toBeGreaterThan(0)
  })

  it('addBook prepends a book and persists to localStorage', () => {
    const before = store.state.books.length
    act(() => {
      store.addBook({
        title: 'The Art of Computer Programming',
        isbn: '9780201896831',
        totalCopies: 2,
        availableCopies: 2,
        branchId: 'cs',
      })
    })
    expect(store.state.books).toHaveLength(before + 1)
    expect(store.state.books[0].title).toBe('The Art of Computer Programming')
    expect(store.state.books[0].id).toMatch(/^bk/)

    const saved = JSON.parse(localStorage.getItem('lms.data.v1'))
    expect(saved.books[0].title).toBe('The Art of Computer Programming')
  })

  it('borrow decreases availability and return restores it', () => {
    const bookId = store.state.books[0].id
    const book = () => store.state.books.find((b) => b.id === bookId)
    const activeLoans = () =>
      store.state.transactions.filter((t) => t.bookId === bookId && t.status === 'issued').length
    const startLoans = activeLoans()

    act(() => {
      store.borrowBook(store.state.students[0].id, bookId)
    })
    expect(activeLoans()).toBe(startLoans + 1)
    expect(book().availableCopies).toBe(book().totalCopies - activeLoans())
    const tx = store.state.transactions.at(-1)
    expect(tx.status).toBe('issued')

    act(() => {
      store.returnBook(tx.id)
    })
    expect(activeLoans()).toBe(startLoans)
    expect(book().availableCopies).toBe(book().totalCopies - activeLoans())
    expect(store.state.transactions.at(-1).status).toBe('returned')
  })

  it('reserves a book once and cancels it', () => {
    const bookId = store.state.books[0].id
    const studentId = store.state.students[0].id

    act(() => {
      store.reserveBook(studentId, bookId)
      store.reserveBook(studentId, bookId) // duplicate is ignored
    })
    expect(store.state.reservations.filter((r) => r.bookId === bookId && r.studentId === studentId)).toHaveLength(1)

    const reservation = store.state.reservations.at(-1)
    act(() => {
      store.cancelReservation(reservation.id)
    })
    expect(store.state.reservations.some((r) => r.id === reservation.id)).toBe(false)
  })

  it('resetData restores the original seed dataset', () => {
    const originalCount = store.state.books.length
    act(() => {
      store.addBook({ title: 'Scratch', isbn: '000', totalCopies: 1, availableCopies: 1 })
      store.resetData()
    })
    expect(store.state.books).toHaveLength(originalCount)
    expect(store.state.books.some((b) => b.title === 'Scratch')).toBe(false)
  })
})
