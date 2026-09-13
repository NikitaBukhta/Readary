# Filtering

Multi-criteria filtering for both book lists: the local library and the online
catalog search. Text search is a separate concern —
[book-search.md](book-search.md) for the local ranking proxy,
[online-book-search.md](online-book-search.md) for the catalogs.

## One criteria object, two lists

`services::BookFilterCriteria` (`src/services/filtering/BookFilterCriteria.hpp`) is a plain
struct — languages, genres, author, publisher, page range, year range, minimum
rating, statuses, edition types — with one method that matters:

```cpp
bool matches(const BookDTO &book) const;
```

**Everything routes through that function.** The local proxy calls it per row;
the composite calls it on every catalog result. There is deliberately no second
implementation of the semantics anywhere.

```
                    BookFilterController  (draft ──apply──▶ applied)
                               │
              ┌────────────────┴────────────────┐
              ▼                                 ▼
  BookController                    GlobalBookSearchController
  kind proxy → criteria proxy       → BookSearchFields.criteria → sources
             → search proxy         → composite re-checks every result
```

`BookFilterController` is the QML singleton. Edits land in a **draft**; only
`apply()` publishes them. Without that split every keystroke in the author field
would re-filter the list and, on the search page, re-issue catalog requests.
`AppInitializer` wires `criteriaApplied` to both consumers — a cross-domain
connect, so it belongs there rather than inside either controller.

`scope` (Library / Search) does **not** change what is filtered — the criteria
apply to both. It only decides which list the facet values are collected from.

One exception to "the same criteria for both lists": status and edition type
describe a book the library owns, and a catalog result has neither. So
`GlobalBookSearchController` stores `criteria.catalogSubset()` — the same object
with those two cleared. Without it, filtering the library by an edition type
would make every online search come back empty, with nothing on screen to
explain why.

## Why the local layer is its own proxy

The chain gained one level:

```
BookListModel → kind proxy (strategy) → BookCriteriaFilterProxyModel → BookSearchProxyModel → QML
```

The criteria could not go on the four kind proxies: every
`BookFilterStrategy::apply()` starts with `clearFilter()`, so the user's criteria
and the category's own filter would share one bucket and "reset filters" would
be ambiguous between them.

`BookCriteriaFilterProxyModel` also does **not** express criteria as
`BookSortFilterProxyModel::addFilter` calls. That would fork the semantics from
the online path and get some of them quietly wrong — matching language with
`Contains("en")` accepts a book whose language cell reads `"fr, eng"`. Instead it
rebuilds the handful of fields the criteria touch out of the row's roles and
calls the same `matches()`.

Two roles had to be added to `BookListModelBase` for this: `LanguageRole` and
`GenresRole`. `BookDTO::language` had been loaded from the DB and populated by
both APIs all along — it simply had no role, so nothing could see it.

## Server-side is an optimisation, not a guarantee

Each catalog gets what it can express, and the composite re-checks everything
afterwards. That is not belt-and-braces; it is required for correctness.

**OpenLibrary's `q` ranks, it does not restrict.** An AND'ed clause constrains
the candidate set while the rest of the query decays into a relevance boost.
Probed live:

| `q` | numFound | top results |
|---|---|---|
| `title:"dune"` | 43125 | Children of Dune, God Emperor of Dune ✓ |
| `title:"dune" AND number_of_pages_median:[400 TO 600]` | 3002 | Dune books, 475–512pp ✓ |
| `title:"dune" AND number_of_pages_median:[100 TO 300]` | 15023 | **Go Ask Alice, Peril at End House** ✗ |
| `title:"dune" AND number_of_pages_median:[900 TO 999]` | 61 | Dune omnibus (920), **then unrelated 993pp** ✗ |
| `title:"dune" AND language:rus` | 93 | Heretics of Dune, **then Crónica de una muerte anunciada** ✗ |
| `title:"dune" AND subject:"science fiction"` | 112 | all Dune ✓ |

No Dune book has 100–300 pages, so with nothing left to rank the title clause
vanishes entirely. **Numeric ranges are therefore never sent to OpenLibrary** —
only the categorical terms (`author:`, `publisher:`, `subject:`, `language:`).

| Criterion | OpenLibrary | Google Books |
|---|---|---|
| language | `language:<marc>` | `langRestrict` |
| author | `author:"…"` | `inauthor:` (Latin only) |
| publisher | `publisher:"…"` | `inpublisher:` (Latin only) |
| genre | `subject:"…"`, OR-joined | `subject:` — single genre only, no OR |
| pages / year / rating | ✗ not sent | ✗ unsupported |
| status / type | local-only concepts | local-only concepts |

Google's field operators match nothing outside Latin script (see
[online-book-search.md](online-book-search.md#googles-field-operators-are-latin-only)),
so a Cyrillic criterion value is dropped from the query rather than sent — it
would turn a filter into an empty result set. The client-side pass still applies
it.

## Consequences worth knowing

**Filtered pages shrink.** A page is 20–25 results and the residual filter can
discard most of them, leaving a near-empty list while `hasMore` is still true.
`GlobalBookSearchController` keeps pulling pages until it has
`g_minFilteredResults` (10) or runs out — bounded by `g_maxAutoPages` (5), or a
filter matching nothing would walk the entire catalog. The stop reason is
logged; a user-driven `loadMore()` resets the budget.

**The search cache is keyed by query *and* criteria** (`cacheKey()`). The same
words under a different filter are a different result set, and serving the
unfiltered one back would look like the filter had failed.

## Genre

Genre had a complete schema (`genres` + `book_genres`, indexed), a read path
(`BookTable::getGenres`) and a UI (`BookGenreTags.qml`) — and no data whatsoever:
no code ever wrote a genre, neither API requested one, and the only seed
(`db/test_data.sql`) is commented out in `AppInitializer`. The pipeline now runs
end to end:

1. **Ingest** — OpenLibrary `subject` (capped at 5 per book; it returns dozens,
   from "Science Fiction" to "Dune (Imaginary place)"), Google `volumeInfo.categories`
   (coarse: "FICTION").
2. **Carry** — `BookDTO::genres`, round-tripped through `toMap`/`fromMap` so the
   search cache preserves them.
3. **Persist** — `BookTable::setGenres()` replaces a book's links, creating genre
   names as needed via `SqlQueryBuilder::insertOrIgnoreInto` (new). Called from
   `addBook`/`updateBook`.
4. **Read back** — `getGenresByBook()` fetches every book's genres in one join
   rather than one `getGenres()` per row.

No schema change was needed, which matters: `init.sql` is `CREATE TABLE IF NOT
EXISTS` only, with no migration mechanism, so an added column would never reach
an existing user's database.

## Facets

The values offered in the sheet are collected from the books already in memory —
the whole library is loaded by `getAllBooks()` anyway and online results never
touch the DB, so no `DISTINCT` query exists or is needed. A language cell holding
`"en, fr"` contributes both codes.

## Files & tests

| File | |
|---|---|
| `src/services/filtering/BookFilterCriteria.{hpp,cpp}` | the criteria and `matches()` |
| `src/models/books/proxy/BookCriteriaFilterProxyModel.{hpp,cpp}` | local filter layer |
| `src/controllers/BookFilterController.{hpp,cpp}` | draft/apply state, facets |
| `qml/components/overlays/FilterSheet.qml` | the panel (ConfirmDialog's Popup recipe) |
| `qml/components/display/FilterChip.qml`, `FilterButton.qml`, `RangeField.qml` | controls |

```bash
ctest --test-dir build/debug -V -R "BookFilterCriteria|BookSearchAPIComposite"
```

`BookFilterCriteriaTest` is the one that matters — it pins the semantics both
paths depend on. Note the fake servers in the API tests answer any query with the
same body, so they can only assert the emitted `q` string, never whether the
catalog agrees with it; re-probe the live endpoints after touching query
generation.
