# Book Search

Book search with relevance-based ranking. Implemented as a proxy model
`BookSearchProxyModel` on top of the flat book list.

## User-facing behavior

- A search field (`AppSearchField`) is present on the main page and on the
  category list page.
- When a query is entered, the list is filtered: only books whose query
  appears as a substring in at least one of **Name**, **Author**, or
  **Description** fields are kept (case-insensitive).
- Matching books are sorted by relevance: the "closer" the match, the higher
  the book in the list.
- An empty query disables filtering and sorting — the list follows the source
  order.

## Architecture

Model chain:

```
BookListModel              data source (DB)
        ↓
BookSortFilterProxyModel   filter by list kind (Want to read / Read / ...)
        ↓
BookSearchProxyModel       search + ranking
        ↓
QML ListView
```

`BookSearchProxyModel` extends `QSortFilterProxyModel`. Its source is the
matching `BookSortFilterProxyModel` selected by `BookController` based on the
active list kind.

Controller: `BookController::searchModel()`. The query is propagated through
`Q_PROPERTY searchQuery`.

## Ranking algorithm

For a non-empty query, each row gets a `matchScore` (`qint8`, range 0…127):

1. Walk fields in priority order (Name → Author → Description).
2. Stop at the **first** field that contains the query (`break`).
3. Score for that field = `coverage × positionFactor × weight`. The result is
   stored in the cache and returned.

### Field weights

| Field | weight |
|-------|--------|
| Name | 100 |
| Author | 60 |
| Description | 20 |

Steep hierarchy: a description match needs ~5× higher
`coverage × positionFactor` to outrank a name match. In practice description
acts as a fallback when name and author both miss.

### `scoreField` formula

```
positionFactor = 1.0 - matchIdx / text.size()
score          = (query.size() / text.size()) * positionFactor * weight
```

- **`coverage`** = `query.size() / text.size()` — how much of the field is
  taken up by the matched substring. Long fields are penalized: the more
  unrelated text surrounds the match, the lower the coverage.
- **`positionFactor`** = `1 - matchIdx / text.size()` — `1.0` if the match
  starts at index 0, decays toward zero if the match is near the end of the
  string.
- **`weight`** — field priority (see table above).

Exact field match (`text == query`): `coverage=1`, `positionFactor=1` →
`score = weight`.

### Sorting and tie-break

Overridden `lessThan`:

1. Empty query — sort by `source row` (preserves source order).
2. Otherwise compare `cachedScore(left.row())` and `cachedScore(right.row())`.
   The greater score is treated as "less" under `Qt::AscendingOrder`, so it
   ends up first.
3. On equal scores — fall back to `source row` for stable order.

Sort is enabled in the constructor via `sort(0, Qt::AscendingOrder)` plus
`setDynamicSortFilter(true)`. On query changes — `invalidate()`, which
re-runs both the filter and the sort.

## Score cache

The `_searchCache` field (`std::vector<qint8>`, mutable) holds one score per
source row. It is populated by `filterAcceptsRow` and read by `lessThan` via
`cachedScore`.

Invalidation:

- **`setSearchQuery`** → `invalidate()` → Qt re-runs `filterAcceptsRow` for
  every row → cache is rewritten.
- **`dataChanged`** on the source → Qt re-invokes `filterAcceptsRow` for the
  affected rows → their cache slots are refreshed.
- **`modelReset`** → Qt re-runs the filter end-to-end → the entire cache is
  rewritten.

### Known issue: `rowsInserted` / `rowsRemoved`

`_searchCache` is keyed by source row index. Qt does **not** call
`filterAcceptsRow` for rows whose indices got shifted by an insert/remove,
so their cache slots remain associated with stale data. Symptom: after a
dynamic insertion into the source, search results may appear in the wrong
order.

**Workaround (when needed):** connect a slot to
`sourceModel()->rowsInserted` / `rowsRemoved`, call `_searchCache.clear()`
(or shift slots accordingly), and `invalidate()`.

This is left out intentionally for now: the source (`BookListModel`) refreshes
through full `refresh()` (model is rebuilt → `modelReset` fires), not via
incremental insert/remove. If that changes, this invalidation must be added.

A NOTE with the same content lives next to `_searchCache` in
`BookSearchProxyModel.hpp`.

## Tests

`src/tests/models/books/proxy/BookSearchProxyModelTest.cpp` — Qt Test.

Covered scenarios:

| Test | What it verifies |
|------|------------------|
| `emptyQuery_passesAllRowsInSourceOrder` | Empty query → all rows in source order |
| `filterDropsNonMatchingRows` | Non-matching rows are filtered out |
| `matchIsCaseInsensitive` | Case-insensitive matching |
| `searchesAcrossNameAuthorAndDescription` | Search works against all three fields |
| `nameMatchOutranksDescriptionMatch` | Name (weight 100) outranks Description (weight 20) |
| `nameMatchOutranksAuthorMatch` | Name (weight 100) outranks Author (weight 60) |
| `earlierMatchPositionRanksHigher` | Equal length/coverage — earlier match wins |
| `higherCoverageRanksHigher` | Equal position — higher coverage wins |
| `exactNameMatchTopsList` | Exact field match takes the top spot |
| `changingQueryUpdatesRanking` | Changing the query rebuilds the cache and order |
| `clearingQueryRestoresAllRows` | Clearing the query brings every row back |
| `dataChangedRefreshesFilterForUpdatedRow` | After `dataChanged` filter and cache are refreshed |
| `modelResetRecomputesEverything` | Full source reset → new ranked output |

`rowsInserted`/`rowsRemoved` are intentionally not covered — matches the
known issue above. Once invalidation is added, add green tests for these too.

### Running

```
cmake --build build --target BookSearchProxyModelTest
ctest --test-dir build -V -R BookSearchProxyModel
```

or run `BookSearchProxyModelTest.exe` directly from the build directory.

## File map

| File | Purpose |
|------|---------|
| [src/models/books/proxy/BookSearchProxyModel.hpp](../../src/models/books/proxy/BookSearchProxyModel.hpp) | Model and cache declaration |
| [src/models/books/proxy/BookSearchProxyModel.cpp](../../src/models/books/proxy/BookSearchProxyModel.cpp) | Filter, scoring and sorting implementation |
| [src/controllers/BookController.cpp](../../src/controllers/BookController.cpp) | Owns `_searchProxy`, binds it to the active list |
| [qml/components/input/AppSearchField.qml](../../qml/components/input/AppSearchField.qml) | Search input UI component |
| [qml/pages/mainPage/MainPage.qml](../../qml/pages/mainPage/MainPage.qml) | Search on the main page |
| [qml/pages/categoryListPage/CategoryListPage.qml](../../qml/pages/categoryListPage/CategoryListPage.qml) | Search inside a category list |
| [src/tests/models/books/proxy/BookSearchProxyModelTest.cpp](../../src/tests/models/books/proxy/BookSearchProxyModelTest.cpp) | Unit tests |
