import { describe, it, expect } from 'vitest'
import { render, screen, fireEvent } from '@testing-library/react'
import { ThemeProvider, useTheme } from './ThemeContext.jsx'

function Probe() {
  const { theme, toggle, setMode } = useTheme()
  return (
    <div>
      <span data-testid="theme">{theme}</span>
      <button onClick={toggle}>toggle</button>
      <button onClick={() => setMode('dark')}>dark</button>
    </div>
  )
}

describe('ThemeContext smoke test', () => {
  it('toggle flips data-theme attribute', () => {
    render(
      <ThemeProvider>
        <Probe />
      </ThemeProvider>,
    )
    expect(document.documentElement.dataset.theme).toBe('light')

    fireEvent.click(screen.getByText('dark'))
    expect(document.documentElement.dataset.theme).toBe('dark')

    fireEvent.click(screen.getByText('toggle'))
    expect(document.documentElement.dataset.theme).toBe('light')

    fireEvent.click(screen.getByText('toggle'))
    expect(document.documentElement.dataset.theme).toBe('dark')
  })
})