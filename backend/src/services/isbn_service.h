/**
 * isbn_service.h - ISBN metadata lookup against the Google Books API.
 *
 * The API key stays on the server (LMS_GOOGLE_BOOKS_API_KEY); the browser only
 * talks to our own /api/books/lookup/:isbn endpoint.
 */
#ifndef LMS_ISBN_SERVICE_H
#define LMS_ISBN_SERVICE_H

#include <stdbool.h>
#include <stddef.h>

#define ISBN_STR_MAX 32
#define ISBN_TITLE_MAX 512
#define ISBN_AUTHOR_MAX 256
#define ISBN_PUBLISHER_MAX 256
#define ISBN_CATEGORY_MAX 128
#define ISBN_DATE_MAX 32
#define ISBN_DESCRIPTION_MAX 2048
#define ISBN_COVER_MAX 1024

typedef struct {
  char isbn[ISBN_STR_MAX];
  char title[ISBN_TITLE_MAX];
  char author[ISBN_AUTHOR_MAX];
  char publisher[ISBN_PUBLISHER_MAX];
  char category[ISBN_CATEGORY_MAX];
  char published_date[ISBN_DATE_MAX];
  char description[ISBN_DESCRIPTION_MAX];
  char cover[ISBN_COVER_MAX];
  int page_count;
  bool found;
} IsbnInfo;

typedef enum {
  ISBN_LOOKUP_OK = 0,
  ISBN_LOOKUP_NOT_FOUND,
  ISBN_LOOKUP_BAD_INPUT,
  ISBN_LOOKUP_UPSTREAM_ERROR
} IsbnLookupStatus;

/**
 * Look up an ISBN-10/13 on Google Books.
 *
 * @param api_key  optional Google Books API key (empty = unauthenticated call)
 * @param isbn     raw ISBN; hyphens and spaces are ignored
 * @param out      filled when ISBN_LOOKUP_OK or ISBN_LOOKUP_NOT_FOUND
 * @param errbuf   receives a human-readable reason on failure
 * @return lookup status; never blocks longer than a few seconds.
 */
IsbnLookupStatus isbn_service_lookup(const char *api_key, const char *isbn,
                                     IsbnInfo *out, char *errbuf, size_t errlen);

#endif /* LMS_ISBN_SERVICE_H */
