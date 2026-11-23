/**
 * isbn_service.c - Google Books ISBN lookup (see isbn_service.h).
 *
 * The HTTP call uses WinHTTP so TLS certificates are validated against the
 * Windows certificate store. The Google API key never leaves the server.
 */
#include "services/isbn_service.h"

#include "config/config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#endif

#define ISBN_JSON_MAX (512 * 1024)
#define ISBN_HTTP_TIMEOUT_MS 8000

static void set_error(char *errbuf, size_t errlen, const char *msg) {
  if (errbuf != NULL && errlen > 0) {
    snprintf(errbuf, errlen, "%s", msg);
  }
}

/* ---- ISBN validation ---------------------------------------------------- */

static bool normalize_isbn(const char *raw, char *out, size_t cap) {
  if (raw == NULL || out == NULL || cap < 2) {
    return false;
  }

  size_t n = 0;
  for (const char *p = raw; *p != '\0'; p++) {
    if (*p == '-' || *p == ' ' || *p == '\t') {
      continue;
    }
    char ch = *p;
    if (ch >= 'a' && ch <= 'z') {
      ch = (char)(ch - 'a' + 'A');
    }
    bool digit = ch >= '0' && ch <= '9';
    bool isbn10_x = ch == 'X' && n == 9;
    if (!digit && !isbn10_x) {
      return false;
    }
    if (n + 1 >= cap) {
      return false;
    }
    out[n++] = ch;
  }
  out[n] = '\0';

  if (n == 13) {
    return strchr(out, 'X') == NULL;
  }
  return n == 10;
}

/* ---- Minimal JSON readers ----------------------------------------------- */

static const char *json_value(const char *json, const char *key) {
  if (json == NULL || key == NULL) {
    return NULL;
  }
  char pattern[64];
  int n = snprintf(pattern, sizeof(pattern), "\"%s\"", key);
  if (n <= 0 || (size_t)n >= sizeof(pattern)) {
    return NULL;
  }

  const char *p = json;
  while ((p = strstr(p, pattern)) != NULL) {
    const char *q = p + (size_t)n;
    while (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r') {
      q++;
    }
    if (*q == ':') {
      q++;
      while (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r') {
        q++;
      }
      return q;
    }
    p += (size_t)n;
  }
  return NULL;
}

static void put_byte(char *out, size_t cap, size_t *n, char ch) {
  if (*n + 1 < cap) {
    out[*n] = ch;
  }
  (*n)++;
}

static void put_utf8(char *out, size_t cap, size_t *n, unsigned int cp) {
  char buf[4];
  size_t len;
  if (cp < 0x80) {
    buf[0] = (char)cp;
    len = 1;
  } else if (cp < 0x800) {
    buf[0] = (char)(0xC0 | (cp >> 6));
    buf[1] = (char)(0x80 | (cp & 0x3F));
    len = 2;
  } else if (cp < 0x10000) {
    buf[0] = (char)(0xE0 | (cp >> 12));
    buf[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
    buf[2] = (char)(0x80 | (cp & 0x3F));
    len = 3;
  } else {
    buf[0] = (char)(0xF0 | (cp >> 18));
    buf[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
    buf[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
    buf[3] = (char)(0x80 | (cp & 0x3F));
    len = 4;
  }
  for (size_t i = 0; i < len; i++) {
    put_byte(out, cap, n, buf[i]);
  }
}

static int hex_value(char ch) {
  if (ch >= '0' && ch <= '9') return ch - '0';
  if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
  if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
  return -1;
}

static bool read_hex4(const char **pp, unsigned int *out) {
  unsigned int cp = 0;
  for (int i = 0; i < 4; i++) {
    int v = hex_value(**pp);
    if (v < 0) return false;
    cp = (cp << 4) | (unsigned int)v;
    (*pp)++;
  }
  *out = cp;
  return true;
}

/** Copies a JSON string starting at *pp (which must point at the quote). */
static bool read_json_string(const char **pp, char *out, size_t cap) {
  const char *p = *pp;
  if (p == NULL || *p != '"' || out == NULL || cap == 0) {
    return false;
  }
  p++;
  size_t n = 0;

  while (*p != '\0' && *p != '"') {
    char ch = *p++;
    if (ch == '\\') {
      char esc = *p;
      if (esc == '\0') {
        return false;
      }
      p++;
      switch (esc) {
        case '"': ch = '"'; break;
        case '\\': ch = '\\'; break;
        case '/': ch = '/'; break;
        case 'b': ch = '\b'; break;
        case 'f': ch = '\f'; break;
        case 'n': ch = '\n'; break;
        case 'r': ch = '\r'; break;
        case 't': ch = '\t'; break;
        case 'u': {
          unsigned int cp = 0;
          if (!read_hex4(&p, &cp)) return false;
          if (cp >= 0xD800 && cp <= 0xDBFF && p[0] == '\\' && p[1] == 'u') {
            const char *save = p;
            p += 2;
            unsigned int low = 0;
            if (read_hex4(&p, &low) && low >= 0xDC00 && low <= 0xDFFF) {
              cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
            } else {
              p = save;
            }
          }
          put_utf8(out, cap, &n, cp);
          continue;
        }
        default: ch = esc; break;
      }
    }
    put_byte(out, cap, &n, ch);
  }

  if (*p != '"') {
    return false;
  }
  size_t end = n < cap ? n : cap - 1;
  out[end] = '\0';
  *pp = p + 1;
  return true;
}

static void get_string(const char *obj, const char *key, char *out, size_t cap) {
  if (out == NULL || cap == 0) {
    return;
  }
  out[0] = '\0';
  const char *v = json_value(obj, key);
  if (v != NULL && *v == '"') {
    const char *p = v;
    (void)read_json_string(&p, out, cap);
  }
}

/** Numbers, booleans and other scalars copied verbatim as text. */
static void get_scalar(const char *obj, const char *key, char *out, size_t cap) {
  if (out == NULL || cap == 0) {
    return;
  }
  out[0] = '\0';
  const char *v = json_value(obj, key);
  if (v == NULL) return;
  size_t n = 0;
  while (*v != '\0' && *v != ',' && *v != '}' && *v != ']' &&
         !isspace((unsigned char)*v) && n + 1 < cap) {
    out[n++] = *v++;
  }
  out[n] = '\0';
}

static const char *get_object(const char *obj, const char *key) {
  const char *v = json_value(obj, key);
  return (v != NULL && *v == '{') ? v : NULL;
}

/** Joins every string in a JSON array, e.g. ["A","B"] -> "A, B". */
static void get_all_array_strings(const char *obj, const char *key, char *out, size_t cap) {
  if (out == NULL || cap == 0) return;
  out[0] = '\0';
  const char *v = json_value(obj, key);
  if (v == NULL || *v != '[') return;

  v++;
  size_t used = 0;
  while (*v != '\0' && used + 1 < cap) {
    while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r' || *v == ',') v++;
    if (*v == ']' || *v != '"') break;
    if (used > 0 && used + 2 < cap) {
      out[used++] = ',';
      out[used++] = ' ';
    }
    char item[ISBN_TITLE_MAX];
    const char *p = v;
    if (!read_json_string(&p, item, sizeof(item))) break;
    size_t len = strlen(item);
    if (used + len + 1 >= cap) break;
    memcpy(out + used, item, len);
    used += len;
    v = p;
  }
  out[used] = '\0';
}

/** Turns industryIdentifiers into "ISBN_13:978...,ISBN_10:...". */
static void get_identifiers(const char *obj, char *out, size_t cap) {
  if (out == NULL || cap == 0) return;
  out[0] = '\0';
  const char *arr = json_value(obj, "industryIdentifiers");
  if (arr == NULL || *arr != '[') return;

  size_t used = 0;
  const char *p = arr;
  while ((p = strstr(p, "\"type\"")) != NULL) {
    const char *tv = p + strlen("\"type\"");
    while (*tv == ' ' || *tv == '\t' || *tv == '\n' || *tv == '\r' || *tv == ':') tv++;
    char type[32];
    if (*tv != '"') break;
    const char *q = tv;
    if (!read_json_string(&q, type, sizeof(type))) break;

    const char *iv = strstr(q, "\"identifier\"");
    if (iv == NULL) break;
    iv += strlen("\"identifier\"");
    while (*iv == ' ' || *iv == '\t' || *iv == '\n' || *iv == '\r' || *iv == ':') iv++;
    char value[64];
    if (*iv != '"') break;
    q = iv;
    if (!read_json_string(&q, value, sizeof(value))) break;

    int n;
    if (used > 0) {
      n = snprintf(out + used, cap - used, ",%s:%s", type, value);
    } else {
      n = snprintf(out + used, cap - used, "%s:%s", type, value);
    }
    if (n < 0 || (size_t)n >= cap - used) break;
    used += (size_t)n;
    p = q;
  }
}

/* ---- HTTP --------------------------------------------------------------- */

#ifdef _WIN32

static void narrow_to_wide(const char *src, wchar_t *dst, size_t cap) {
  size_t i = 0;
  for (; src[i] != '\0' && i + 1 < cap; i++) {
    dst[i] = (wchar_t)(unsigned char)src[i];
  }
  dst[i] = L'\0';
}

static bool https_get_json(const char *isbn, const char *api_key, char *out,
                           size_t cap, char *errbuf, size_t errlen) {
  wchar_t wisbn[ISBN_STR_MAX];
  wchar_t wkey[LMS_MAX_STR];
  wchar_t path[1024];

  narrow_to_wide(isbn, wisbn, sizeof(wisbn) / sizeof(wisbn[0]));
  narrow_to_wide(api_key != NULL ? api_key : "", wkey, sizeof(wkey) / sizeof(wkey[0]));

  int plen;
  if (wkey[0] != L'\0') {
    plen = swprintf(path, sizeof(path) / sizeof(path[0]),
                    L"/books/v1/volumes?q=isbn:%ls&key=%ls", wisbn, wkey);
  } else {
    plen = swprintf(path, sizeof(path) / sizeof(path[0]),
                    L"/books/v1/volumes?q=isbn:%ls", wisbn);
  }
  if (plen < 0) {
    set_error(errbuf, errlen, "The ISBN lookup URL is too long.");
    return false;
  }

  HINTERNET session = WinHttpOpen(L"lms-backend/0.1",
                                  WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                  WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  if (session == NULL) {
    set_error(errbuf, errlen, "Could not initialize the HTTPS client.");
    return false;
  }
  WinHttpSetTimeouts(session, ISBN_HTTP_TIMEOUT_MS, ISBN_HTTP_TIMEOUT_MS,
                     ISBN_HTTP_TIMEOUT_MS, ISBN_HTTP_TIMEOUT_MS);

  bool ok = false;
  HINTERNET connect = NULL;
  HINTERNET request = NULL;

  connect = WinHttpConnect(session, L"www.googleapis.com",
                           INTERNET_DEFAULT_HTTPS_PORT, 0);
  if (connect == NULL) {
    set_error(errbuf, errlen, "Could not reach Google Books.");
    goto done;
  }

  request = WinHttpOpenRequest(connect, L"GET", path, NULL, WINHTTP_NO_REFERER,
                               WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
  if (request == NULL) {
    set_error(errbuf, errlen, "Could not build the Google Books request.");
    goto done;
  }

  if (!WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                          WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
      !WinHttpReceiveResponse(request, NULL)) {
    snprintf(errbuf, errlen, "Google Books request failed (error %lu).",
             (unsigned long)GetLastError());
    goto done;
  }

  DWORD status = 0;
  DWORD status_len = sizeof(status);
  if (!WinHttpQueryHeaders(request,
                           WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                           WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_len,
                           WINHTTP_NO_HEADER_INDEX)) {
    set_error(errbuf, errlen, "Could not read the Google Books response status.");
    goto done;
  }
  if (status != 200) {
    snprintf(errbuf, errlen, "Google Books returned HTTP %lu.", (unsigned long)status);
    goto done;
  }

  size_t total = 0;
  while (total + 1 < cap) {
    DWORD got = 0;
    DWORD want = (DWORD)(cap - 1 - total);
    if (!WinHttpReadData(request, out + total, want, &got)) {
      snprintf(errbuf, errlen, "Could not read the Google Books response (error %lu).",
               (unsigned long)GetLastError());
      goto done;
    }
    if (got == 0) {
      break;
    }
    total += got;
  }
  out[total] = '\0';
  ok = true;

done:
  if (request != NULL) WinHttpCloseHandle(request);
  if (connect != NULL) WinHttpCloseHandle(connect);
  WinHttpCloseHandle(session);
  return ok;
}

#else

static bool https_get_json(const char *isbn, const char *api_key, char *out,
                           size_t cap, char *errbuf, size_t errlen) {
  (void)isbn;
  (void)api_key;
  (void)out;
  (void)cap;
  set_error(errbuf, errlen, "Google Books lookup is only available on Windows builds.");
  return false;
}

#endif

/* ---- Public API --------------------------------------------------------- */

static void promote_to_https(char *url) {
  if (url == NULL) {
    return;
  }
  if (strncmp(url, "http://", 7) == 0) {
    size_t rest = strlen(url + 7);
    memmove(url + 8, url + 7, rest + 1);
    memcpy(url, "https://", 8);
  }
}

IsbnLookupStatus isbn_service_lookup(const char *api_key, const char *isbn,
                                      IsbnInfo *out, char *errbuf, size_t errlen) {
  if (out == NULL) {
    set_error(errbuf, errlen, "Missing lookup output.");
    return ISBN_LOOKUP_BAD_INPUT;
  }
  memset(out, 0, sizeof(*out));

  char clean[ISBN_STR_MAX];
  if (!normalize_isbn(isbn, clean, sizeof(clean))) {
    set_error(errbuf, errlen, "ISBN must be 10 or 13 valid digits.");
    return ISBN_LOOKUP_BAD_INPUT;
  }
  snprintf(out->isbn, sizeof(out->isbn), "%s", clean);

  char *json = malloc(ISBN_JSON_MAX);
  if (json == NULL) {
    set_error(errbuf, errlen, "The server ran out of memory.");
    return ISBN_LOOKUP_UPSTREAM_ERROR;
  }

  if (!https_get_json(clean, api_key, json, ISBN_JSON_MAX, errbuf, errlen)) {
    free(json);
    return ISBN_LOOKUP_UPSTREAM_ERROR;
  }

  const char *items = json_value(json, "items");
  const char *volume = items != NULL && *items == '[' ? json_value(items, "volumeInfo") : NULL;
  if (volume == NULL || *volume != '{') {
    free(json);
    out->found = false;
    return ISBN_LOOKUP_NOT_FOUND;
  }

  get_string(json, "id", out->volume_id, sizeof(out->volume_id));
  get_string(volume, "title", out->title, sizeof(out->title));
  if (out->title[0] == '\0') {
    free(json);
    out->found = false;
    return ISBN_LOOKUP_NOT_FOUND;
  }

  get_string(volume, "subtitle", out->subtitle, sizeof(out->subtitle));
  get_string(volume, "authors", out->author, sizeof(out->author));
  get_all_array_strings(volume, "authors", out->authors, sizeof(out->authors));
  if (out->authors[0] == '\0') {
    snprintf(out->authors, sizeof(out->authors), "%s", out->author);
  }
  get_string(volume, "categories", out->category, sizeof(out->category));
  get_all_array_strings(volume, "categories", out->categories, sizeof(out->categories));
  if (out->categories[0] == '\0') {
    snprintf(out->categories, sizeof(out->categories), "%s", out->category);
  }
  get_identifiers(volume, out->identifiers, sizeof(out->identifiers));
  get_string(volume, "publisher", out->publisher, sizeof(out->publisher));
  get_string(volume, "publishedDate", out->published_date, sizeof(out->published_date));
  get_string(volume, "description", out->description, sizeof(out->description));
  get_string(volume, "language", out->language, sizeof(out->language));
  get_string(volume, "printType", out->print_type, sizeof(out->print_type));
  get_string(volume, "maturityRating", out->maturity_rating, sizeof(out->maturity_rating));
  get_scalar(volume, "averageRating", out->average_rating, sizeof(out->average_rating));
  get_scalar(volume, "ratingsCount", out->ratings_count, sizeof(out->ratings_count));
  get_scalar(volume, "pageCount", out->page_count, sizeof(out->page_count));
  get_string(volume, "previewLink", out->preview_link, sizeof(out->preview_link));
  get_string(volume, "infoLink", out->info_link, sizeof(out->info_link));
  get_string(volume, "canonicalVolumeLink", out->canonical_link, sizeof(out->canonical_link));

  const char *links = get_object(volume, "imageLinks");
  if (links != NULL) {
    get_string(links, "thumbnail", out->cover, sizeof(out->cover));
    if (out->cover[0] == '\0') {
      get_string(links, "smallThumbnail", out->cover, sizeof(out->cover));
    }
    get_string(links, "extraLarge", out->cover_large, sizeof(out->cover_large));
    if (out->cover_large[0] == '\0') get_string(links, "large", out->cover_large, sizeof(out->cover_large));
    if (out->cover_large[0] == '\0') get_string(links, "medium", out->cover_large, sizeof(out->cover_large));
    if (out->cover_large[0] == '\0') {
      snprintf(out->cover_large, sizeof(out->cover_large), "%s", out->cover);
    }
  }
  promote_to_https(out->cover);
  promote_to_https(out->cover_large);

  const char *dims = get_object(volume, "dimensions");
  if (dims != NULL) {
    char h[32] = {0}, w[32] = {0}, t[32] = {0};
    get_string(dims, "height", h, sizeof(h));
    get_string(dims, "width", w, sizeof(w));
    get_string(dims, "thickness", t, sizeof(t));
    if (h[0] != '\0' && w[0] != '\0') {
      snprintf(out->dimensions, sizeof(out->dimensions), "%s x %s%s%s", h, w,
               t[0] ? " x " : "", t[0] ? t : "");
    }
  }

  const char *search = get_object(json, "searchInfo");
  if (search != NULL) {
    get_string(search, "textSnippet", out->snippet, sizeof(out->snippet));
  }

  const char *sale = get_object(json, "saleInfo");
  if (sale != NULL) {
    get_string(sale, "saleability", out->saleability, sizeof(out->saleability));
    get_string(sale, "isEbook", out->is_ebook, sizeof(out->is_ebook));
    get_string(sale, "buyLink", out->buy_link, sizeof(out->buy_link));
    const char *price = get_object(sale, "listPrice");
    if (price != NULL) {
      char amount[32] = {0}, currency[16] = {0};
      get_string(price, "amount", amount, sizeof(amount));
      get_string(price, "currencyCode", currency, sizeof(currency));
      if (amount[0] != '\0') {
        snprintf(out->list_price, sizeof(out->list_price), "%s %s", amount, currency);
      }
    }
  }

  const char *access = get_object(json, "accessInfo");
  if (access != NULL) {
    get_string(access, "viewability", out->viewability, sizeof(out->viewability));
    get_string(access, "webReaderLink", out->web_reader_link, sizeof(out->web_reader_link));
    const char *epub = get_object(access, "epub");
    if (epub != NULL) get_string(epub, "isAvailable", out->epub_available, sizeof(out->epub_available));
    const char *pdf = get_object(access, "pdf");
    if (pdf != NULL) get_string(pdf, "isAvailable", out->pdf_available, sizeof(out->pdf_available));
  }

  out->found = true;
  free(json);
  return ISBN_LOOKUP_OK;
}

static size_t append_meta_field(char *out, size_t cap, size_t used,
                                const char *key, const char *value) {
  if (used + 1 >= cap) return used;
  int n = snprintf(out + used, cap - used, "\"%s\":\"", key);
  if (n < 0) return used;
  used += (size_t)n;

  for (const char *p = value != NULL ? value : ""; *p != '\0' && used + 7 < cap; p++) {
    unsigned char ch = (unsigned char)*p;
    switch (ch) {
      case '"': memcpy(out + used, "\\\"", 2); used += 2; break;
      case '\\': memcpy(out + used, "\\\\", 2); used += 2; break;
      case '\n': memcpy(out + used, "\\n", 2); used += 2; break;
      case '\r': memcpy(out + used, "\\r", 2); used += 2; break;
      case '\t': memcpy(out + used, "\\t", 2); used += 2; break;
      default:
        if (ch < 0x20) {
          used += (size_t)snprintf(out + used, cap - used, "\\u%04x", ch);
        } else {
          out[used++] = (char)ch;
        }
        break;
    }
  }
  if (used + 2 < cap) {
    out[used++] = '"';
    out[used++] = ',';
  }
  return used;
}

void isbn_service_metadata_json(const IsbnInfo *info, char *out, size_t cap) {
  if (out == NULL || cap == 0) return;
  out[0] = '{';
  size_t used = 1;

#define META_FIELD(key, value) used = append_meta_field(out, cap, used, key, value)
  META_FIELD("isbn", info->isbn);
  META_FIELD("volumeId", info->volume_id);
  META_FIELD("title", info->title);
  META_FIELD("subtitle", info->subtitle);
  META_FIELD("author", info->author);
  META_FIELD("authors", info->authors);
  META_FIELD("category", info->category);
  META_FIELD("categories", info->categories);
  META_FIELD("industryIdentifiers", info->identifiers);
  META_FIELD("publisher", info->publisher);
  META_FIELD("publishedDate", info->published_date);
  META_FIELD("description", info->description);
  META_FIELD("textSnippet", info->snippet);
  META_FIELD("cover", info->cover);
  META_FIELD("coverLarge", info->cover_large);
  META_FIELD("language", info->language);
  META_FIELD("printType", info->print_type);
  META_FIELD("maturityRating", info->maturity_rating);
  META_FIELD("averageRating", info->average_rating);
  META_FIELD("ratingsCount", info->ratings_count);
  META_FIELD("pageCount", info->page_count);
  META_FIELD("dimensions", info->dimensions);
  META_FIELD("previewLink", info->preview_link);
  META_FIELD("infoLink", info->info_link);
  META_FIELD("canonicalVolumeLink", info->canonical_link);
  META_FIELD("saleability", info->saleability);
  META_FIELD("isEbook", info->is_ebook);
  META_FIELD("listPrice", info->list_price);
  META_FIELD("buyLink", info->buy_link);
  META_FIELD("viewability", info->viewability);
  META_FIELD("epubAvailable", info->epub_available);
  META_FIELD("pdfAvailable", info->pdf_available);
  META_FIELD("webReaderLink", info->web_reader_link);
#undef META_FIELD

  if (used > 1 && out[used - 1] == ',') out[used - 1] = '\0';
  if (used + 1 < cap) {
    strcat(out, "}");
  }
}
