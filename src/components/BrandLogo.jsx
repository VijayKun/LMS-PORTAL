import { useState } from 'react'
import { LibraryBig } from 'lucide-react'

const SOURCES = ['/logo.png', '/logo.svg', '/logo.jpg', '/logo.jpeg']

/**
 * College logo shown in the sidebar and login brand blocks.
 * Drop a file named logo.png / logo.svg / logo.jpg into public/ and it
 * replaces the default icon automatically. If no file exists yet (or it
 * fails to load), the default library icon is shown instead.
 */
export default function BrandLogo({ size = 19 }) {
  const [index, setIndex] = useState(0)

  if (index >= SOURCES.length) return <LibraryBig size={size} />

  return (
    <img
      src={SOURCES[index]}
      alt=""
      className="brand-logo-img"
      onError={() => setIndex((i) => i + 1)}
    />
  )
}
