# Models and Filters

Models are organized by domain under `src/models/`:

- `src/models/books/` — book-related models (this document)
- `src/models/books/filters/` — strategy hierarchy for book filters
- `src/models/settings/` — user-preference models (e.g. `LanguageModel`,
  see [i18n.md](i18n.md))

Four book models plus a strategy hierarchy live under `src/models/books/`.
Search behavior is documented separately in
[book-search.md](book-search.md); this document covers the rest.

## `BookListModel`

Plain `QAbstractListModel` over a `QList<services::BookDTO>`. Owned by
`AppInitializer` and passed by pointer to `BookController`.

- `refresh()` reloads the list from `BookTable::getAllBooks()` and triggers
  `modelReset` so all proxies recompute. Called on app start and after a book
  is saved (`BookController::bookSaved` → `BookListModel::refresh`).
- `getBook(qint64 id) const` — `std::find_if` over the local cache, returns
  the matching plain `BookDTO`. Used by `BookController::currentBookData`,
  which wraps the result in a `BookDTOObject` (a Q_GADGET subclass that
  adds the `genres` field for the detail view). Characters are loaded by
  a separate `BookCharactersModel`, not folded into this struct.
- `deleteBook(qint64 id)` — `Q_INVOKABLE`, deletes via `BookTable` and
  refreshes. Sets `errorMessage` on failure.

### Roles

```
IdRole, NameRole, AuthorIdRole, AuthorRole, YearRole, PublisherIdRole,
PublisherRole, DescriptionRole, CoverUrlRole,
IsHardcoverRole, TypeIdRole, TypeRole,
TotalPagesRole, PagesReadRole,
GlobalRatingRole, LocalRatingRole, UserRatingRole,
StatusRole, InWishListRole
```

QML role names (`roleNames()`) — one-to-one with the enum, names match the
DTO keys (`bookId`, `name`, `author`, `year`, `coverUrl`, `totalPages`,
`pagesRead`, `globalRating`, `localRating`, `userRating`, `status`,
`inWishList`, …). See [`BookListModel.cpp::roleNames`](../../src/models/books/BookListModel.cpp)
for the full mapping.

`BookListModel` is **not** a QML-visible type — QML always reaches data
through one of the proxies on `BookController`.

## `BookSortFilterProxyModel`

Generic `QSortFilterProxyModel` parameterized by a flexible filter API.
Marked `QML_ANONYMOUS` so QML can resolve property types but cannot
instantiate it directly.

### Filter API

```cpp
proxy.addFilter(role, value, Op::Equal);   // Op defaults to Equal
proxy.removeFilter(role);                   // remove all filters for that role
proxy.clearFilter();                        // remove everything
```

Available operators (`Op`, `Q_ENUM`):
`Equal`, `NotEqual`, `Less`, `LessOrEqual`, `Greater`, `GreaterOrEqual`,
`Contains`.

**Combination semantics:**

- Filters on the **same role** are OR'ed together (covers "status IN (1, 2)"
  type queries).
- Filters on **different roles** are AND'ed together.

Implemented by iterating `_filters.uniqueKeys()`, taking the equal_range per
role, and rejecting the row if any role has no matching filter.

### Sort

- `setSortField(role)` (`Q_PROPERTY sortField`) — picks which role to sort
  by. Default: `BookListModel::NameRole`.
- `setSortDescending(bool)` (`Q_PROPERTY sortDescending`).
- `lessThan` policy:
  - `YearRole`, `TotalPagesRole`, `PagesReadRole` — `toInt()` compare;
  - `GlobalRatingRole`, `LocalRatingRole`, `UserRatingRole` — `toDouble()`
    compare (ratings are stored as REAL in SQL except `userRating`);
  - everything else — `QString::localeAwareCompare`.

### `count` property

Q_PROPERTY `int count READ rowCount NOTIFY countChanged`. The model
reconnects `rowsInserted/rowsRemoved/modelReset/layoutChanged` to a
`countChanged` signal so QML bindings on `count` re-evaluate reactively.

Used by main-page categories to show "%n book(s)" subtitles.

## `BookCharactersModel`

Standalone `QAbstractListModel` over `QList<services::CharacterDTO>` for
the current book's characters. Owned by `BookController` (one instance,
auto-synced to `currentBookId`). Roles: `id`, `name`, `role`. Exists as
a separate model — and not as another field of `currentBookData` —
because the detail page lists characters one by one with virtualization,
and a "Show more" UX gate, neither of which fits inside a value-type
`BookDTO` snapshot.

API:

- `setBookId(qint64)` — runs `BookTable::getCharacters(id)` once
  (one full SELECT, no LIMIT/OFFSET), stores all rows in `_allItems`,
  exposes the first 5 via `_visibleCount`. Internal: invoked by
  `BookController` on every `currentBookIdChanged`.
- `loadMore()` (`Q_INVOKABLE`) — advances `_visibleCount` by another
  page (5) through `beginInsertRows`/`endInsertRows`, so the QML
  `ListView` only sees the new rows arriving (no full reset, no
  scroll jump).
- `hide()` (`Q_INVOKABLE`) — collapses `_visibleCount` back to the
  initial page (5) through `beginRemoveRows`/`endRemoveRows`. Mirror
  of `loadMore`.
- `canLoadMore` / `canHide` (both `Q_PROPERTY` with NOTIFY) — true when
  there are still buffered rows / the visible window has been expanded
  past the first page. Drive the "Show more" / "Show less" button
  visibility on the detail page.

Why one query upfront, not paginated SQL: a typical book has on the
order of dozens of characters; one `SELECT id, name, role FROM
book_characters WHERE book_id = ?` is sub-millisecond on SQLite, and
keeping all rows in memory costs single-digit KB. Multiple round-trips
would only add latency.

## `ReadingHistoryModel`

Same shape as `BookCharactersModel`, over
`QList<services::ReadingSessionDTO>`: one `BookTable::getReadingSessions(isbn)`
SELECT (closed sessions only, newest first), all rows kept in memory, the first
5 exposed through `_visibleCount`, and the identical `loadMore()` / `hide()` /
`canLoadMore` / `canHide` paging contract — so QML drives both lists with the
same `PagedListToggle` component.

Roles: `id`, `startedAt`, `endedAt`, `pagesFrom`, `pagesTo`, `pagesRead`,
`durationSeconds` (the last two derived by the DTO, not stored).

Two things differ from the characters model:

- `totalCount` (`Q_PROPERTY`, `summaryChanged`) describes the whole journal
  rather than the visible window; the detail page uses it to hide the section
  for a book that was never read.
- `setBookIsbn(isbn)` with the ISBN *already* in place keeps however far the
  list was expanded; only switching books collapses back to the first page.
  `BookController` re-sets the same ISBN after `bookSaved` (a saved session
  appends a row), and a reload must not yank an expanded list shut.

## Strategy pattern

Configuration of the four sort/filter proxies (one per `ListKind`) is done
through `BookFilterStrategy` (`src/models/books/filters/`). The strategy
wraps "how to configure a `BookSortFilterProxyModel` for this category".

### Hierarchy

```cpp
class BookFilterStrategy {
public:
  virtual ~BookFilterStrategy() = default;
  virtual void apply(BookSortFilterProxyModel *proxy) const = 0;
};

class WantToReadFilterStrategy   : public BookFilterStrategy { ... };
class WantToBuyFilterStrategy    : public BookFilterStrategy { ... };
class AlreadyReadFilterStrategy  : public BookFilterStrategy { ... };
class ReadInProgressFilterStrategy : public BookFilterStrategy { ... };
```

### Concrete strategies

| Strategy | What it does |
|----------|--------------|
| `WantToReadFilterStrategy` | `addFilter(StatusRole, 1)` — status == WantToRead |
| `ReadInProgressFilterStrategy` | `addFilter(StatusRole, 2)` — status == InProgress |
| `AlreadyReadFilterStrategy` | `addFilter(StatusRole, 3)` — status == Finished |
| `WantToBuyFilterStrategy` | `addFilter(InWishListRole, true)` |

Each strategy starts with `proxy->clearFilter()` so the proxy's prior state
is irrelevant — strategies are idempotent.

### How `BookController` uses them

```cpp
BookSortFilterProxyModel *
BookController::buildProxy(BookListModel *source,
                           const BookFilterStrategy &strategy) {
  auto *proxy = new BookSortFilterProxyModel(this);
  proxy->setSourceModel(source);
  strategy.apply(proxy);
  return proxy;
}
```

Adding a fifth list = adding a new strategy class + a new `ListKind` entry +
one `_proxies.insert(...)` line. No changes to `BookSortFilterProxyModel`.

### Why Strategy and not State

Strategies don't transition between each other and they don't carry the
"active list" state. The active list is held by `BookController` (the
context); switching is just "look up a different proxy in the QHash". So the
classification problem is "configure the algorithm for this category", which
is Strategy. The user's earlier State-pattern attempt forced a
context/state lifecycle that didn't exist here.

## File map

| File | Purpose |
|------|---------|
| [src/models/books/BookListModel.hpp](../../src/models/books/BookListModel.hpp) / [.cpp](../../src/models/books/BookListModel.cpp) | Source model over the books table |
| [src/models/books/BookSortFilterProxyModel.hpp](../../src/models/books/BookSortFilterProxyModel.hpp) / [.cpp](../../src/models/books/BookSortFilterProxyModel.cpp) | Generic filter/sort proxy with `addFilter`/`Op` API |
| [src/models/books/BookSearchProxyModel.hpp](../../src/models/books/BookSearchProxyModel.hpp) / [.cpp](../../src/models/books/BookSearchProxyModel.cpp) | Search + relevance ranking — see [book-search.md](book-search.md) |
| [src/models/books/BookCharactersModel.hpp](../../src/models/books/BookCharactersModel.hpp) / [.cpp](../../src/models/books/BookCharactersModel.cpp) | Per-book characters list with windowed reveal |
| [src/models/books/ReadingHistoryModel.hpp](../../src/models/books/ReadingHistoryModel.hpp) / [.cpp](../../src/models/books/ReadingHistoryModel.cpp) | Per-book reading-session journal with windowed reveal |
| [src/models/books/filters/BookFilterStrategy.hpp](../../src/models/books/filters/BookFilterStrategy.hpp) / [.cpp](../../src/models/books/filters/BookFilterStrategy.cpp) | Strategy interface + 4 concrete strategies |
