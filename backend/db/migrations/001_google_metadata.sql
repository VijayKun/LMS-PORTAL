-- Stores the complete Google Books volumeInfo record for each catalog book.
ALTER TABLE public.books
  ADD COLUMN IF NOT EXISTS google_metadata jsonb NOT NULL DEFAULT '{}'::jsonb;
