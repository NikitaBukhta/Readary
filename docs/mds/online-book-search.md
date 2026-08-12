# Online book search

How a query typed into the discovery UI reaches the public catalogs and comes
back as `BookDTO`s. This is the *online* half of search — the in-library
substring/ranking proxy is a separate thing, documented in
[book-search.md](book-search.md).

```
QML  →  GlobalBookSearchController  →  BookSearchAPIComposite  →  OpenLibrarySeachAPI   (primary)
                                                               ↘  GoogleBooksSearchAPI  (fallback)
                                              ↘ IBookNetSearchAPI (shared HTTP transport + retry)
```

## The sources

| | Primary | Fallback |
|---|---|---|
| Class | `OpenLibrarySeachAPI` | `GoogleBooksSearchAPI` |
| Endpoint | `https://openlibrary.org/search.json` | `https://www.googleapis.com/books/v1/volumes` |
| Page size | 25 | 20 |
| Auth | none | optional `key=` ([see below](#api-credentials)) |
| Work key | the raw path, `/works/OL1W` | `gbooks:` + volume id |
| Language filter | `language:<marc>` inside `q` | `langRestrict=<iso639-1>` |
| Docs | [openlibrary.org/dev/docs/api/search](https://openlibrary.org/dev/docs/api/search) | [developers.google.com/books/docs/v1](https://developers.google.com/books/docs/v1/using) |

Both implement `IBookSearchAPI` (`search` / `searchByISBN` / `fetchDescription`
→ `searchListUpdated` / `descriptionReady`) and both drop any result lacking a
valid ISBN or a page count: without an ISBN a result can't be reconciled with
internal books, without a page count the reading-progress UI has nothing to
work with. `hasMore` is inferred from `returned == pageSize`.

Each source's **work-key prefix is what makes the fan-out safe**.
`fetchDescription` is broadcast to every source, and each ignores a key it
didn't produce (`/works/` vs `gbooks:`), so exactly one HTTP request goes out.

## Primary → fallback

`BookSearchAPIComposite` used to fan out to all sources in parallel and re-emit
a growing aggregate on every reply — N emissions per search, all of them
partial. It is now a small state machine that emits **exactly once**:

```
                 ┌─────────── all primaries replied, aggregate non-empty ──────────┐
                 │                                                                 ▼
Idle ──search──▶ AwaitingPrimary ──all replied, aggregate empty──▶ AwaitingFallback ──▶ finish() ──▶ Idle
```

- `_pendingPrimary` counts outstanding primary replies. It and `_stage` are set
  **before** dispatching, because a source may answer synchronously and
  re-enter `handlePrimaryResult` inside the dispatch loop.
- The fallback **replaces** the (empty) primary result, it never merges.
- `_params` is remembered so the fallback repeats the same query, including the
  page number and language filter.

### The stale-reply guard

`_stage` is not bookkeeping, it's the correctness mechanism. Two searches
dispatched before the first reply lands (a double-fire from the UI) means two
primary replies are in flight for one composite. Without the guard the second
one arrives while the composite is in `AwaitingFallback`, gets mistaken for the
*fallback's* answer, and re-triggers the fallback — two queries, two emissions,
duplicated results in the model.

Primary and fallback replies land in **separate slots**, each of which drops
anything arriving in the wrong stage. `BookSearchApiCompositeTest::overlappingSearch_ignoresStalePrimaryReply_queriesFallbackOnce`
pins this down.

### Known limit

`loadMore()` re-enters `search()` with `page = N`, so a query served by the
fallback pays one wasted primary round trip per extra page. Correct, just
chatty — remembering the winning source per query would fix it.

A second source of duplicate traffic is fixed: `startFreshSearch` clears the
results model, the QML list re-evaluates its end-of-list trigger on that reset
and calls `loadMore()` **synchronously** — previously before `requestPage()` had
set `_loading`, so every fresh search asked for page 1 twice (visible in the log
as `loadMore — page: 1` right after `search query`, then `dropping stale primary
reply`). The controller now claims `_loading` before touching the model.

## Language filter

The composite fills `BookSearchFields::language` (ISO 639-1) from the query
text when the caller left it empty; each source then expresses it in its own
dialect via `LanguageConverter` (`toMarc` for OpenLibrary, ISO 639-1 as-is for
Google). `searchByISBN` is never filtered — an ISBN already identifies one
edition.

Detection is script-based (`api::detectQueryLanguage`, see
[description-translation.md](description-translation.md#detecting-a-language)):

- **Cyrillic** → `uk` if the text contains `і ї є ґ`, else `ru`.
- **Anything else** → empty, i.e. **no filter**. Latin script spans en/de/fr and
  dozens more; filtering on a guess would hide valid editions, and the cost of
  guessing wrong is a search that silently returns nothing.

Note that OpenLibrary's field terms are OR'd together (the controller sends the
same text as both title and author), so the language clause has to be AND'd
onto the group — `(title:"…" OR author:"…") AND language:rus`. OR'ing it in
would *widen* the search.

Multi-word field values are quoted (`title:"war and peace"`) through the shared
`IBookNetSearchAPI::asFieldValue`; without it both catalogs scope only the first
token to the field and drop the rest to a default match.

### Google's field operators are Latin-only

The controller sends the user's text as **both** name and author, meaning "match
either" — and Google's `intitle:`/`inauthor:` keywords match nothing at all once
the text leaves Latin script. Probed live:

| `q` (`langRestrict=ru`) | totalItems |
|---|---|
| `inauthor:"анджей сапковский"` | **0** |
| `inauthor:"Анджей Сапковский"` (proper case) | **0** |
| `intitle:"анджей сапковский"` | **0** |
| `"анджей сапковский"` (unqualified) | **300** |
| `intitle:"последнее желание"` | 300 |

It is not case and not the language filter. An **unqualified phrase** matches
title and author alike, in either script; `inauthor:` silently matches nothing
outside Latin, and `intitle:` happens to work for Cyrillic *titles* — which is
why searching a Russian title looked fine while searching a Russian author
returned an empty list.

So `GoogleBooksSearchAPI::generateQuery` sends the "match either" case as a bare
(quoted, when multi-word) phrase with no field prefix. A caller supplying a
genuinely different name and author still gets `intitle:… OR inauthor:…`.
OpenLibrary's fields do handle Cyrillic and keep the OR form.

This is invisible to the tests — the fake server answers any query with the same
body, so they can only pin the emitted `q` string — so it is worth re-probing the
live endpoint after touching query generation.

OpenLibrary's *coverage* of Cyrillic is thin regardless of query shape (one
Sapkowski record, no page count, so it is dropped). That is data, not a query
bug: the composite falling through to Google Books is the intended path.

### Misses are not cached

`GlobalBookSearchController` persists every completed search, but an **empty**
result is no longer treated as a cache hit (neither in memory nor from
`SearchCache`). A search comes back with nothing when the catalogs were merely
throttled, or when a query shape has since been fixed — serving that back would
freeze the miss in for `g_searchCacheMaxAgeDays` (7). Any query that returned
nothing is simply re-issued.

## Transport and retry

`IBookNetSearchAPI` owns one `QNetworkAccessManager` per source and is
`GET`-only and fully async. `QNetworkAccessManager::finished` goes to a private
`onReplyFinished`, **not** straight to the `responseReceived` signal, so
subclasses only ever see a terminal reply.

- Retriable: HTTP 429/500/502/503/504, plus timeout and temporary-network
  errors. Both catalogs answer with transient 5xx under load, and treating that
  as "no books found" would send every such search down the fallback path.
- Not retriable: a refused connection or a DNS failure — repeating it only
  stalls an offline run.
- 3 attempts, 200 ms × attempt backoff. `QHash<QNetworkReply*, int> _attempts`
  holds the per-request state.

Replies are correlated to handlers by **URL path sniffing**, not request
identity (`/works/…` vs `/search.json`; `…/volumes` vs `…/volumes/{id}`). This
is why two concurrent searches against the same source can't be told apart, and
why the composite's stale-reply guard exists at all.

## API credentials

`secrets/api_keys.env` (gitignored; copy `api_keys.env.example`) is parsed at
**configure** time by `cmake/ReadarySecrets.cmake` and baked in as a compile
definition:

```cmake
readary_load_secrets(${_SECRETS_FILE} ReadaryCore GOOGLE_BOOKS_API_KEY)
```

`GoogleBooksSearchAPI` reads it through `#ifdef GOOGLE_BOOKS_API_KEY` and simply
omits `key=` when it isn't defined — a missing file or empty value is a STATUS
message, not an error, and the client falls back to the anonymous (lower)
quota. The file is listed in `CMAKE_CONFIGURE_DEPENDS`, so editing a key
reconfigures instead of sitting behind a stale cache.

Two things to keep in mind: the key **ships inside the binary** and can be
extracted from it, so restrict it by application in the Google Cloud console;
and request URLs are logged with `QUrl::RemoveQuery`, which is what keeps the
key (and the user's query text) out of the log file.

## Files & tests

| File | |
|---|---|
| `src/api/bookSearch/IBookSearchAPI.hpp` | interface + `BookSearchFields` |
| `src/api/bookSearch/IBookNetSearchAPI.{hpp,cpp}` | HTTP transport, retry, `asFieldValue` |
| `src/api/bookSearch/BookSearchApiComposite.{hpp,cpp}` | primary/fallback state machine |
| `src/api/bookSearch/OpenLibrarySeachAPI.{hpp,cpp}` | primary source |
| `src/api/bookSearch/GoogleBooksSearchAPI.{hpp,cpp}` | fallback source |
| `cmake/ReadarySecrets.cmake` | env file → compile definitions |

Tests use a local `QTcpServer` stand-in rather than the real network; both API
suites redirect the client with `setEndpoint()`, which exists for that purpose.

```bash
ctest --test-dir build/debug -V -R "BookSearchApiComposite|GoogleBooksSearchAPI|OpenLibrarySearchAPI"
```
