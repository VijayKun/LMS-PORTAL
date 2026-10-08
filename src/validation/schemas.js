/**
 * Zod schemas for the API boundary.
 *
 * These validate untrusted data on the frontend so the mock service layer -
 * and later the real fetch() calls - only ever see well-formed payloads.
 * When the mock API is replaced, keep calling these before every request:
 * `parseWith(schema, payload)` throws a plain Error on invalid input.
 */
import { z } from 'zod'

/** Shared primitives. */
const emailOrStudentId = z.string().trim().min(1).max(200)
const shortText = (max) => z.string().trim().max(max)

export const loginRequestSchema = z.object({
  email: emailOrStudentId,
  role: z.enum(['student', 'admin']),
})

/** One validated row of a bulk catalog import. Mirrors importRows.js caps. */
export const importRowSchema = z.object({
  title: shortText(300),
  isbn: z.string().trim().min(1).max(30),
  author: shortText(200),
  category: shortText(100),
  publisher: shortText(200),
  copies: z.number().int().min(1).max(9999),
  branchId: z.string().nullish(),
})

export const importRowsSchema = z.array(importRowSchema).max(5000)

/** Parses `payload` with `schema`, throwing a plain Error on failure. */
export function parseWith(schema, payload) {
  const result = schema.safeParse(payload)
  if (!result.success) {
    const first = result.error.issues?.[0]
    throw new Error(`Invalid request payload${first ? `: ${first.path.join('.')} ${first.message}` : ''}`)
  }
  return result.data
}
