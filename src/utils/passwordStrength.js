/**
 * Lightweight password strength heuristic for the login/reset UI.
 * Advisory only - the server must enforce real rules (length, breach
 * checks, rate limiting). No dependencies, no zxcvbn.
 *
 * Returns { score: 0..3, label, tone } where tone maps to CSS modifier.
 */
export function passwordStrength(pw) {
  const value = String(pw ?? '')
  if (!value) return { score: 0, label: 'Enter a password', tone: 'weak' }

  let score = 0
  if (value.length >= 8) score++
  if (value.length >= 12) score++
  const classes =
    (/[a-z]/.test(value) ? 1 : 0) +
    (/[A-Z]/.test(value) ? 1 : 0) +
    (/\d/.test(value) ? 1 : 0) +
    (/[^\w\s]/.test(value) ? 1 : 0)
  if (classes >= 3) score++
  if (/^(.)\1+$/.test(value) || /^(?:0123456789|1234567890|qwertyuiop|password)/i.test(value)) {
    score = 0
  }

  const capped = Math.min(3, score)
  if (capped <= 0) return { score: 0, label: 'Weak', tone: 'weak' }
  if (capped === 1) return { score: 1, label: 'Weak', tone: 'weak' }
  if (capped === 2) return { score: 2, label: 'Fair', tone: 'fair' }
  return { score: 3, label: 'Strong', tone: 'strong' }
}
