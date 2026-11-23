/**
 * isbn_service.h - ISBN metadata lookup against the Google Books API.
 *
 * The API key stays on the server (LMS_GOOGLE_BOOKS_API_KEY); the browser only
 * talks to our own /api/books/lookup/:isbn endpoint. The full Google Books
 * record is captured in IsbnInfo and serialised by isbn_service_metadata_json.
 */
#ifndef LMS_ISBN_SERVICE_H
#define LMS_ISBN_SERVICE_H

#include <stdbool.h>
#include <stddef.h>

#define ISBN_STR_MAX 32
#define ISBN_TITLE_MAX 512
#define ISBN_SUBTITLE_MAX 512
#define ISBN_AUTHOR_MAX 512
#define ISBN_CATEGORY_MAX 256
#define ISBN_IDENTIFIERS_MAX 256
#define ISBN_PUBLISHER_MAX 256
#define ISBN_DATE_MAX 32
#define ISBN_DESCRIPTION_MAX 4096
#define ISBN_COVER_MAX 1024
#define ISBN_LANGUAGE_MAX 32
#define ISBN_PRINT_TYPE_MAX 32
#define ISBN_MATURITY_MAX 32
#define ISBN_RATING_MAX 32
#define ISBN_DIMENSIONS_MAX 128
#define ISBN_LINK_MAX 1024
#define ISBN_VOLUME_ID_MAX 64
#define ISBN_META_JSON_MAX 32768

typedef struct {
  char isbn[ISBN_STR_MAX];
  char volume_id[ISBN_VOLUME_ID_MAX];
  char title[ISBN_TITLE_MAX];
  char subtitle[ISBN_SUBTITLE_MAX];
  char author[ISBN_AUTHOR_MAX];
  char authors[ISBN_AUTHOR_MAX];
  char category[ISBN_CATEGORY_MAX];
  char categories[ISBN_CATEGORY_MAX];
  char identifiers[ISBN_IDENTIFIERS_MAX];
  char publisher[ISBN_PUBLISHER_MAX];
  char published_date[ISBN_DATE_MAX];
  char description[ISBN_DESCRIPTION_MAX];
  char snippet[ISBN_DESCRIPTION_MAX];
  char cover[ISBN_COVER_MAX];
  char cover_large[ISBN_COVER_MAX];
  char language[ISBN_LANGUAGE_MAX];
  char print_type[ISBN_PRINT_TYPE_MAX];
  char maturity_rating[ISBN_MATURITY_MAX];
  char average_rating[ISBN_RATING_MAX];
  char ratings_count[ISBN_RATING_MAX];
  char page_count[ISBN_RATING_MAX];
  char dimensions[ISBN_DIMENSIONS_MAX];
  char preview_link[ISBN_LINK_MAX];
  char info_link[ISBN_LINK_MAX];
  char canonical_link[ISBN_LINK_MAX];
  char saleability[ISBN_MATURITY_MAX];
  char is_ebook[ISBN_RATING_MAX];
  char list_price[ISBN_RATING_MAX];
  char buy_link[ISBN_LINK_MAX];
  char viewability[ISBN_MATURITY_MAX];
  char epub_available[ISBN_RATING_MAX];
  char pdf_available[ISBN_RATING_MAX];
  char web_reader_link[ISBN_LINK_MAX];
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

/** Serialise every captured field as a JSON object for the API response. */
void isbn_service_metadata_json(const IsbnInfo *info, char *out, size_t cap);

#endif /* LMS_ISBN_SERVICE_H */
