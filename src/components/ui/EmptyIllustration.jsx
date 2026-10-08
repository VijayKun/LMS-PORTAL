/**
 * Friendly vector illustrations for empty states.
 * Purely decorative - the icon-based EmptyState look stays available by
 * simply not passing a `kind`.
 */
export default function EmptyIllustration({ kind = 'default' }) {
  if (kind === 'search') return <SearchScene />
  if (kind === 'books') return <BooksScene />
  if (kind === 'bell') return <BellScene />
  return <OpenBookScene />
}

function Frame({ children }) {
  return (
    <svg className="empty-illu" viewBox="0 0 160 110" width="160" height="110" aria-hidden="true">
      {children}
    </svg>
  )
}

function SearchScene() {
  return (
    <Frame>
      <ellipse cx="80" cy="96" rx="58" ry="7" fill="var(--hover)" />
      <rect x="30" y="30" width="70" height="54" rx="8" fill="var(--primary-soft)" stroke="var(--border-strong)" strokeWidth="2" />
      <path d="M42 44h46M42 54h34M42 64h40" stroke="var(--primary)" strokeWidth="3" strokeLinecap="round" opacity="0.55" />
      <circle cx="104" cy="62" r="22" fill="var(--card)" stroke="var(--primary)" strokeWidth="4" />
      <path d="M120 78l14 14" stroke="var(--primary)" strokeWidth="6" strokeLinecap="round" />
      <path d="M96 62a8 8 0 0 1 8-8" stroke="var(--primary)" strokeWidth="3" strokeLinecap="round" fill="none" opacity="0.7" />
      <circle cx="130" cy="30" r="4" fill="var(--warning)" opacity="0.8" />
      <circle cx="40" cy="20" r="3" fill="var(--primary)" opacity="0.5" />
    </Frame>
  )
}

function BooksScene() {
  return (
    <Frame>
      <ellipse cx="80" cy="96" rx="58" ry="7" fill="var(--hover)" />
      <rect x="34" y="66" width="92" height="18" rx="5" fill="var(--primary)" opacity="0.85" />
      <rect x="42" y="48" width="76" height="18" rx="5" fill="var(--primary-soft)" stroke="var(--border-strong)" strokeWidth="2" />
      <rect x="50" y="30" width="60" height="18" rx="5" fill="var(--card)" stroke="var(--border-strong)" strokeWidth="2" />
      <path d="M60 30v18M100 48v18" stroke="var(--border-strong)" strokeWidth="2" />
      <path d="M92 30h10v12l-5-4-5 4z" fill="var(--warning)" />
      <path d="M56 56h16" stroke="var(--primary)" strokeWidth="3" strokeLinecap="round" opacity="0.6" />
      <path d="M50 74h28" stroke="#fff" strokeWidth="3" strokeLinecap="round" opacity="0.7" />
    </Frame>
  )
}

function BellScene() {
  return (
    <Frame>
      <ellipse cx="80" cy="96" rx="52" ry="7" fill="var(--hover)" />
      <path
        d="M80 24c-14 0-24 10-24 24v14l-8 12h64l-8-12V48c0-14-10-24-24-24z"
        fill="var(--primary-soft)"
        stroke="var(--primary)"
        strokeWidth="3"
        strokeLinejoin="round"
      />
      <path d="M80 16v8" stroke="var(--primary)" strokeWidth="3" strokeLinecap="round" />
      <circle cx="80" cy="14" r="5" fill="var(--primary)" />
      <path d="M70 82a10 10 0 0 0 20 0" fill="none" stroke="var(--primary)" strokeWidth="3" strokeLinecap="round" />
      <circle cx="118" cy="34" r="4" fill="var(--warning)" />
      <circle cx="44" cy="30" r="3" fill="var(--success)" opacity="0.8" />
      <path d="M124 52l2 4 4 2-4 2-2 4-2-4-4-2 4-2z" fill="var(--warning)" opacity="0.7" />
    </Frame>
  )
}

function OpenBookScene() {
  return (
    <Frame>
      <ellipse cx="80" cy="96" rx="58" ry="7" fill="var(--hover)" />
      <path
        d="M80 36c-10-8-24-10-40-8v46c16-2 30 0 40 8 10-8 24-10 40-8V28c-16-2-30 0-40 8z"
        fill="var(--primary-soft)"
        stroke="var(--primary)"
        strokeWidth="3"
        strokeLinejoin="round"
      />
      <path d="M80 36v46" stroke="var(--primary)" strokeWidth="3" />
      <path d="M52 46c8-1 16 0 20 3M52 58c8-1 16 0 20 3M88 49c8-3 16-4 24-3M88 61c8-3 16-4 24-3" stroke="var(--primary)" strokeWidth="2.5" strokeLinecap="round" opacity="0.5" fill="none" />
      <circle cx="126" cy="26" r="4" fill="var(--warning)" opacity="0.75" />
      <circle cx="36" cy="24" r="3" fill="var(--primary)" opacity="0.5" />
    </Frame>
  )
}
