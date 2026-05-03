# Controllers

Two QML singletons live in `src/controllers/`. Both follow the same pattern:
the C++ instance is created by `AppInitializer`, registered via
`setInstance()`, and the QML factory `create()` returns the prepared instance
with `QQmlEngine::CppOwnership`.

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
| `currentBookData` | property (RO, `QVariantMap`) | scalar fields from `BookListModel::getBook(id)` augmented with `genres` (`QStringList`) and `characters` (`QVariantList`) from `BookTable`; cached per id |
| `errorMessage` | property (RO, `QString`) | last validation/error string |
| `activeKind` | property (R/W, `ListKind`) | which category is currently active |
| `searchModel` | property (RO, `BookSearchProxyModel*`) | what QML lists bind to |
| `getSortFilterProxyForKind(kind)` | `Q_INVOKABLE` | proxy for a specific kind |
| `openBook(id)` | `Q_INVOKABLE` | sets `currentBookId` and emits `bookOpenRequested(id)` for the router to pick up |
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

The getter performs two side-table reads (`getGenres`, `getCharacters`) per
`currentBookId`. To avoid hitting the DB on every QML binding evaluation:

- `mutable QVariantMap _cachedBookData;` is filled lazily on first read.
- Cleared in `setCurrentBookId(id)` when id changes.
- Cleared on `_listModel->modelReset` (followed by re-emitting
  `currentBookIdChanged` so QML re-evaluates with fresh data).

`currentBookId` and `currentBookData` share the `currentBookIdChanged`
NOTIFY signal — semantically slightly fuzzy but functionally correct.

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
PROFILE_PAGE       = 6   level 1   (placeholder — falls back to MAIN_PAGE)
BOOK_DETAIL_PAGE   = 7   level 3   qrc:/qt/qml/Library/pages/bookDetailPage/BookDetailPage.qml
```

The four placeholder pages (Search/Goals/Challenges/Profile) are exposed so
`BottomNavBar` can drive `currentPage` to them, but their `pageInfo()` entry
maps to `MAIN_PAGE`'s URL — they'll get real implementations later.

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
