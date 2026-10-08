import { createContext, useContext, useEffect } from 'react'
import { INSTITUTION_NAME } from '../data/settings.js'

/**
 * Lets any page set the title (and optional subtitle) shown in the top bar:
 *   usePageTitle('Inventory', 'Manage books and library collection')
 */
export const PageTitleContext = createContext(null)

export function usePageTitle(title, subtitle) {
  const setMeta = useContext(PageTitleContext)

  useEffect(() => {
    setMeta?.({ title, subtitle: subtitle ?? null })
    document.title = `${title} | LMS - ${INSTITUTION_NAME}`
    return () => setMeta?.(null)
  }, [title, subtitle, setMeta])
}
