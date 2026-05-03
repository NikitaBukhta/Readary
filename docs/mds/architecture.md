# Architecture

The app is split into four layers, with strict downward dependencies:

```
QML (Library module)        ← pages, reusable components, theme
   ↑ properties / signals / Q_INVOKABLE
Controllers                 ← BookController, NavigationController
   ↑ owns / observes
Models                      ← BookListModel + proxies + filter strategies
   ↑ uses
Services                    ← BookTable (CRUD over BookDTO)
   ↑ uses
Core                        ← DatabaseManager, SqlQueryBuilder, AppEnvironment
```

QML never reaches into models or services directly — controllers are the only
QML-visible entry points (singletons). Models are exposed as properties on the
controllers.

## Object lifecycle

`AppInitializer` (in `src/core/`) wires everything up at startup:

1. `initDatabase()` — opens SQLite via `DatabaseManager`, runs `:/db/init.sql`,
   and in debug builds also `:/db/test_data.sql`. Creates `BookTable`.
2. `initModels()` — creates `BookListModel`, then `BookController` (which
   internally creates the search proxy and the four sort/filter proxies),
   then `NavigationController`. Wires:
   - `BookController::bookSaved` → `BookListModel::refresh`
   - `BookController::bookOpenRequested` → `NavigationController::setCurrentPage(BOOK_DETAIL_PAGE)`
     (so `BookController::openBook(id)` is the single entry to the detail
     page; the controllers don't include each other directly).
3. `registerQmlTypes()` — calls `setInstance()` on each QML-singleton class
   so the `create()` factory the QML engine invokes returns the prepared C++
   instance with `QQmlEngine::CppOwnership`.

All Qt objects are parented to `AppInitializer` so they're destroyed before
`QQmlApplicationEngine` shuts down.

## Source list / search / category list (data flow)

```
   BookListModel               (one source — books from DB, refreshed on edits)
        │
        ├── BookSortFilterProxyModel #1   (filter: WantToRead)
        ├── BookSortFilterProxyModel #2   (filter: InProgress)
        ├── BookSortFilterProxyModel #3   (filter: AlreadyRead)
        └── BookSortFilterProxyModel #4   (filter: WantToBuy)
                          │
                          ▼ (the active proxy is set as source)
                BookSearchProxyModel       (free-text search + relevance sort)
                          │
                          ▼
                       QML view
```

- `BookController` owns all four `BookSortFilterProxyModel` instances and one
  `BookSearchProxyModel`.
- The "active list" is selected via `BookController.activeKind`; when it
  changes, the search proxy's source model is swapped to the matching
  sort/filter proxy.
- The search proxy is the model QML binds to — it always reflects the active
  category plus the user's search query.

Each sort/filter proxy is configured by a **filter strategy** (Strategy
pattern) at construction time; see [models-and-filters.md](models-and-filters.md).

## Why singletons (and why only at the controller layer)

- Controllers are global session state (the app has one form being edited at
  a time, one navigation stack, etc.) — singletons fit.
- Models are not global by design: they're owned by their controller and
  exposed as properties (`BookController.searchModel`, etc.). This keeps QML
  away from raw data classes.

## Top-level directory map

```
src/
  core/        AppEnvironment, AppInitializer, DatabaseManager, SqlQueryBuilder
  services/    BookTable, BookDTO
  models/      BookListModel, BookSearchProxyModel, BookSortFilterProxyModel
    filters/   BookFilterStrategy + 4 concrete strategies
  controllers/ BookController, NavigationController
  tests/       Qt Test units (BookSearchProxyModelTest, etc.)
qml/
  Main.qml     Window root, holds page Loader
  pages/
    mainPage/         MainPage + sections (Currently reading, Categories, …)
    categoryListPage/ CategoryListPage (vertical book list per category)
    bookDetailPage/   BookDetailPage + header / progress / ratings / characters cards
  components/  Reusable UI: SurfaceCard / PressableSurface / PaddedCard / TouchTarget
                base components, plus rows, buttons, search field, nav bar,
                progress widgets, StarRating, TagPill
  theme/       Theme singleton + palettes (Pink/Blue/Yellow/Purple)
  utils/       Geometry, Styles (singletons with named-component sub-specs)
db/
  db_scripts.qrc  Resource manifest for SQL scripts
  init.sql        Schema (books + side tables: genres, book_genres,
                  book_characters, reading_sessions)
  test_data.sql   Seed data (debug only)
```
