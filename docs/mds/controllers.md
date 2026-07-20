# Controllers

Three QML singletons live in `src/controllers/`. All follow the same pattern:
the C++ instance is created by `AppInitializer`, registered via
`setInstance()`, and the QML factory `create()` returns the prepared instance
with `QQmlEngine::CppOwnership`.

- `BookController` — book form/list state, character model, reading-timer entry points.
- `NavigationController` — page stack and routing.
- `SettingsController` — façade for user preferences. Owns
  `LanguageModel` (and any future settings models). See
  [i18n.md](i18n.md) for the language pipeline end-to-end.

## `BookController`

The single entry point for QML to anything book-related. It mixes two
responsibilities deliberately, since both are book-scoped and small:

- **Form state** — currently selected book id, derived data, error message.
- **List state** — which list (`activeKind`) is currently shown, plus the
  search proxy that QML binds to.

### QML-visible API

| Member | Kind | Purpose |
|--------|------|---------|
| `currentBookId` | property (R/W, `qint64`) | id of the book being shown/edited; 0 = none |
| `currentBookData` | property (RO, `bl::qmltypes::BookDTOObject` Q_GADGET) | typed snapshot of the current book: scalar fields inherited from `BookDTO` plus `genres` (`QStringList`) loaded from `BookTable`; cached per id. Characters are NOT here — see `charactersModel` |
| `errorMessage` | property (RO, `QString`) | last validation/error string |
| `activeKind` | property (R/W, `ListKind`) | which category is currently active |
| `searchModel` | property (RO, `BookSearchProxyModel*`) | what QML lists bind to |
| `charactersModel` | property (RO, `BookCharactersModel*`) | per-current-book character list with paged exposure (`canLoadMore` / `loadMore()` to expand by 5; `canHide` / `hide()` to collapse back to first page); auto-syncs when `currentBookId` changes |
| `getSortFilterProxyForKind(kind)` | `Q_INVOKABLE` | proxy for a specific kind |
| `openBook(id)` | `Q_INVOKABLE` | sets `currentBookId` and emits `bookOpenRequested(id)` for the router to pick up |
| `updateReadingProgress(pageNumber, durationSeconds)` | `Q_INVOKABLE` | persists current reading position in `books.pagesRead` and logs a row in `reading_sessions`; emits `bookSaved` so the list refreshes |
| `setBookStatus(status)` | `Q_INVOKABLE` | writes `books.status` for the current book (values from `BookStatus`); emits `bookSaved` |
| `toggleWantToRead()` | `Q_INVOKABLE` | flips the current book's status between `WantToRead` and `None`. Not meant for an in-progress book — QML routes that case through the move-warning dialog to `moveInProgressToWantToRead()` |
| `toggleWishList()` | `Q_INVOKABLE` | flips `books.inWishList` (the "want to buy" flag); independent of `status` |
| `moveInProgressToWantToRead()` | `Q_INVOKABLE` | for an in-progress book: snapshots `pagesRead` into `ReadingProgressCache` (only when > 0), clears the reading-timer cache, then resets `pagesRead` to 0 and sets status `WantToRead`. Confirmed via a warning dialog because it discards visible progress |
| `hasCachedProgress()` | `Q_INVOKABLE` (const) | true if the current book has a cached progress snapshot (i.e. it was moved out of in-progress). Drives the "restore progress?" prompt shown when reading is (re)started |
| `restoreCachedProgress()` | `Q_INVOKABLE` | takes-and-clears the cached snapshot, writing it back to `books.pagesRead` and setting status `InProgress`; emits `bookSaved` |
| `discardCachedProgress()` | `Q_INVOKABLE` (const) | drops the cached snapshot without restoring (the "start over" choice) |
| `saveReadingSession(bookIsbn, seconds, phase)` | `Q_INVOKABLE static` | per-book timer-state writer; parses `bookIsbn` (string) and forwards to `services::ReadingSessionCache::save`. Static because there's no instance state — the ISBN is explicit |
| `takeReadingSession(bookIsbn)` | `Q_INVOKABLE static` | reads-and-clears the timer-state group via `ReadingSessionCache::takeState`. Returns `{}` if nothing saved |
| `clearReadingSession(bookIsbn)` | `Q_INVOKABLE static` | drops the timer-state group |
| `bookSaved` | signal | emitted after a successful save; wired to `BookListModel::refresh` |
| `bookOpenRequested(qint64 id)` | signal | wired in `AppInitializer` to `NavigationController::setCurrentPage(BOOK_DETAIL_PAGE)` |

`ListKind` enum (`Q_ENUM`):

```
WantToRead = 0   // status == 1 in the DB
WantToBuy  = 1   // inWishList == true
AlreadyRead= 2   // status == 3
InProgress = 3   // status == 2 (used by "Currently reading" section)
```

QML accesses values as `BookController.WantToRead`, etc.

For the `status` enum used on the detail page (`BookStatus.Finished`,
`BookStatus.InProgress`, …), see [`BookStatus`](../../src/services/BookStatus.hpp)
— it's a separate Q_GADGET to avoid name clashes with `ListKind` members.

For the reading-timer phase (`ReadingPhase.Stopped` / `Running` / `Paused`),
see [`ReadingPhase`](../../src/services/ReadingPhase.hpp) — also Q_GADGET,
single source of truth shared between `ReadingSessionCache` (C++) and
`ReadingProgressTimer` (QML).

### `openBook(id)` flow

QML callsites use a single Q_INVOKABLE instead of duplicating the pair
"set id + change page":

```qml
// in any list delegate
onClicked: BookController.openBook(model.bookId)
```

Internally:
```cpp
void BookController::openBook(qint64 id) {
  if (id <= 0) return;
  setCurrentBookId(id);              // updates state, invalidates cache
  emit bookOpenRequested(id);        // AppInitializer routes this to navigation
}
```

The signal is wired in [`AppInitializer::initModels`](../../src/core/AppInitializer.cpp):

```cpp
connect(_bookController, &BookController::bookOpenRequested, _contextModel,
        [this](qint64) {
          _contextModel->setCurrentPage(NavigationController::PageEnum::BOOK_DETAIL_PAGE);
        });
```

This decouples `BookController` from `NavigationController` (no
cross-include between controllers).

### `currentBookData` caching

The getter performs one side-table read (`getGenres`) per `currentBookId`.
To avoid hitting the DB on every QML binding evaluation:

- `mutable BookDTOObject _cachedBookData;` + `mutable bool _cacheValid` are
  filled lazily on first read.
- Invalidated in `setCurrentBookId(id)` when id changes.
- Invalidated on `_listModel->modelReset` (followed by re-emitting
  `currentBookIdChanged` so QML re-evaluates with fresh data).

`currentBookId` and `currentBookData` share the `currentBookIdChanged`
NOTIFY signal — semantically slightly fuzzy but functionally correct.

### `charactersModel`

A [`BookCharactersModel`](../../src/models/books/BookCharactersModel.hpp)
(`QAbstractListModel`) owned by the controller and auto-synced to
`currentBookId` via the same `currentBookIdChanged` signal: on every
emit the model calls `setBookId(currentBookId)` which reloads the full
list from `BookTable::getCharacters` in one SQL query, then exposes
the first 5 rows. `loadMore()` advances the visible window by another
page through `beginInsertRows`/`endInsertRows`; `hide()` collapses it
back to the first page through `beginRemoveRows`/`endRemoveRows` —
no extra DB queries, no full model reset, no scroll jump. `canLoadMore`
and `canHide` (`Q_PROPERTY`s) drive the QML "Show more" / "Show less"
button visibility.

### Reading-timer flow

The `ReadingProgressTimer` QML component drives a stopwatch over the current
reading session. State (`seconds`, `phase`) is persisted via
[`ReadingSessionCache`](../../src/services/ReadingSessionCache.hpp) (QSettings
under `readingSession/<bookId>/`) so it survives app restarts.

Two distinct paths through `BookController`:

- **Per-tick state** — `saveReadingSession`/`takeReadingSession`/`clearReadingSession`
  proxy to `ReadingSessionCache::*`. They take the ISBN explicitly so the QML
  side can capture it at component creation and use the same value at
  destruction even if `currentBookIsbn` shifts in between. Static because they
  carry no controller state of their own. The ISBN is passed as a **string**,
  not `qint64`: a 13-digit ISBN silently arrives as `0` through a `qint64` QML
  invokable parameter (see [qml.md](qml.md) and the note in
  `ReadingProgressTimer.qml`), so these methods parse the string back to
  `qint64` internally.
- **Final progress** — when the user confirms the page on session end,
  `updateReadingProgress(pageNumber, durationSeconds)` runs a real DB write:
  - `BookTable::updatePagesRead` → `UPDATE books SET pagesRead = ?`
  - `BookTable::insertReadingSession` → `INSERT INTO reading_sessions (...)`
    with `started_at = now − duration`. Failure here is logged but does not
    block the primary update.
  - `emit bookSaved` triggers `BookListModel::refresh`, which rebuilds the
    list (and our cache invalidates), so any QML binding on
    `currentBookData.pagesRead` reflects the new value immediately.

### Internal layout

The constructor builds four `BookSortFilterProxyModel` instances, one per
`ListKind`, and configures each with the corresponding filter strategy
(see [models-and-filters.md](models-and-filters.md#strategy-pattern)):

```cpp
_proxies.insert(ListKind::WantToRead, buildProxy(_listModel, WantToReadFilterStrategy{}));
_proxies.insert(ListKind::WantToBuy,  buildProxy(_listModel, WantToBuyFilterStrategy{}));
_proxies.insert(ListKind::AlreadyRead,buildProxy(_listModel, AlreadyReadFilterStrategy{}));
_proxies.insert(ListKind::InProgress, buildProxy(_listModel, ReadInProgressFilterStrategy{}));

applyActiveSourceToSearchProxy();   // _searchProxy->setSourceModel(activeProxy)
```

When `setActiveKind(kind)` runs, it updates `_activeKind` and reapplies the
source on `_searchProxy`. From QML this is a single property assignment:

```qml
BookController.activeKind = BookController.WantToBuy
```

### File map

| File | Purpose |
|------|---------|
| [src/controllers/BookController.hpp](../../src/controllers/BookController.hpp) | Class declaration, properties, enum, signals |
| [src/controllers/BookController.cpp](../../src/controllers/BookController.cpp) | Wiring of proxies, activeKind logic, form state, cache |

## `NavigationController`

Tiny stack-based router. Holds a `QStack<PageEnum>`; pushing a page that
already sits below the top is rejected (no duplicate pushes). Levels enforce
hierarchy — a higher-level page replaces lower-or-equal levels on push, so
pushing `MAIN_PAGE` (level 1) over `CATEGORY_LIST_PAGE` (level 2) collapses
back to the root.

### QML-visible API

| Member | Kind | Purpose |
|--------|------|---------|
| `currentPage` | property (R/W, `PageEnum`) | top of the stack |
| `currentPagePath` | property (RO, QUrl) | qrc URL of the QML for the top page |
| `goBack()` | `Q_INVOKABLE` | pops one page; no-op if stack has 1 entry |

`PageEnum`:

```
MAIN_PAGE          = 1   level 1   qrc:/qt/qml/Library/pages/mainPage/MainPage.qml
CATEGORY_LIST_PAGE = 2   level 2   qrc:/qt/qml/Library/pages/categoryListPage/CategoryListPage.qml
SEARCH_PAGE        = 3   level 1   (placeholder — falls back to MAIN_PAGE)
GOALS_PAGE         = 4   level 1   (placeholder — falls back to MAIN_PAGE)
CHALLENGES_PAGE    = 5   level 1   (placeholder — falls back to MAIN_PAGE)
PROFILE_PAGE       = 6   level 1   qrc:/qt/qml/Library/pages/settingsPage/SettingsPage.qml
BOOK_DETAIL_PAGE   = 7   level 3   qrc:/qt/qml/Library/pages/bookDetailPage/BookDetailPage.qml
```

The three remaining placeholder pages (Search/Goals/Challenges) are exposed
so `BottomNavBar` can drive `currentPage` to them, but their `pageInfo()`
entry maps to `MAIN_PAGE`'s URL — they'll get real implementations later.
`PROFILE_PAGE` already routes to the
[Settings page](../../qml/pages/settingsPage/SettingsPage.qml) (language picker
+ future preferences).

### Wiring in QML

Top-level `Main.qml` holds a `Loader` whose `source` is bound to
`NavigationController.currentPagePath`. Anything that wants to navigate sets
`NavigationController.currentPage = …` (or calls `goBack()`).

For book detail entry the canonical path is `BookController.openBook(id)`,
not direct `currentPage = BOOK_DETAIL_PAGE` — see the section above.

### File map

| File | Purpose |
|------|---------|
| [src/controllers/NavigationController.hpp](../../src/controllers/NavigationController.hpp) | Enum + properties |
| [src/controllers/NavigationController.cpp](../../src/controllers/NavigationController.cpp) | Stack logic, page→URL mapping |
| [qml/Main.qml](../../qml/Main.qml) | `Loader.source: NavigationController.currentPagePath` |
