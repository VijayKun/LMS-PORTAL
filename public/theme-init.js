// Apply the stored theme before first paint to avoid a flash of the wrong theme.
// External file (not inline) so the Content-Security-Policy can stay at script-src 'self'.
try {
  document.documentElement.dataset.theme =
    localStorage.getItem('lms.theme') === 'dark' ? 'dark' : 'light'
} catch {
  document.documentElement.dataset.theme = 'light'
}
