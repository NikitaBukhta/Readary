# Architecture

The app is split into four layers, with strict downward dependencies:

```
QML (Library module)        ← pages, reusable components, theme
   ↑ properties / signals / Q_INVOKABLE
Controllers                 ← BookController, NavigationController, SettingsController
   ↑ owns / observes
Models                      ← books/   — BookListModel + proxies + filter strategies
                              settings/ — LanguageModel
   ↑ uses                            ↘ returns
Services                    ← BookTable (CRUD over BookDTO)
   ↑ uses                       wraps ↗
QML types                   ← Q_GADGET wrappers (BookDTOObject) — moc-touched value types exposed at the controller→QML boundary
   ↑ uses
Core                        ← DatabaseManager, SqlQueryBuilder, AppEnvironment
```

`src/qmltypes/` is a thin layer between services and controllers. It
holds value types (Q_GADGETs) that adapt service-layer DTOs for QML:
they inherit from the plain DTO and add `Q_PROPERTY MEMBER` aliases
(plus any view-only fields like `genres` for the book detail page).
The data layer stays free of moc/Q_GADGET, while controllers can
return a typed gadget that QML reads members directly off.

QML never reaches into models or services directly — controllers are the only
QML-visible entry points (singletons). Models are exposed as properties on the
controllers. Two Q_GADGET enum-namespaces — [`BookStatus`](../../src/services/BookStatus.hpp)
and [`ReadingPhase`](../../src/services/ReadingPhase.hpp) — are also QML-visible
through `QML_ELEMENT`, but only as type registrations (no instances).

## Object lifecycle

`AppInitializer` (in `src/core/`) wires everything up at startup:

1. `initDatabase()` — opens SQLite via `DatabaseManager`, runs `:/db/init.sql`,
   and in debug builds also `:/db/test_data.sql`. Creates `BookTable`.
2. `initModels()` — creates `BookListModel`, then `BookController` (which
   internally creates the search proxy and the four sort/filter proxies),
   then `NavigationController`, then `SettingsController` (which
   constructs `LanguageModel` as its child). Wires:
   - `BookController::bookSaved` → `BookListModel::refresh`
   - `BookController::bookOpenRequested` → `NavigationController::setCurrentPage(BookDetailPage)`
     (so `BookController::openBook(id)` is the single entry to the detail
     page; the controllers don't include each other directly).
   - `LanguageModel::currentChanged` → `_engine->retranslate()` with
     `Qt::QueuedConnection` so retranslate runs after the QML setter that
     triggered the change has unwound. Also calls
     `_settingsController->languageModel()->applyCurrent()` once at startup
     so the initial `QTranslator` matches persisted `QSettings` (or the
     host locale on first launch). See [i18n.md](i18n.md) for the rest of
     the pipeline.
3. `registerQmlTypes()` — calls `setInstance()` on each QML-singleton class
   so the `create()` factory the QML engine invokes returns the prepared C++
   instance with `QQmlEngine::CppOwnership`.

All Qt objects are parented to `AppInitializer` so they're destroyed before
`QQmlApplicationEngine` shuts down.

[`main.cpp`](../../src/main.cpp) sets `QGuiApplication::setOrganizationName /
setOrganizationDomain / setApplicationName` *before* constructing
`AppInitializer`. This anchors `QSettings` (used by `ReadingSessionCache`
for cross-launch reading-timer persistence) to a stable per-user location
regardless of the binary name or build kind.

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
  services/    BookTable, BookDTO, CharacterDTO, ReadingSessionDTO, BookStatus, ReadingPhase, ReadingSessionCache
  qmltypes/    BookDTOObject (Q_GADGET wrapper over BookDTO, exposed by BookController to QML)
  models/
    books/     BookListModel, BookSearchProxyModel, BookSortFilterProxyModel, BookCharactersModel,
               ReadingHistoryModel
      filters/ BookFilterStrategy + 4 concrete strategies
    settings/  LanguageModel
  controllers/ BookController, NavigationController, SettingsController
  tests/       Qt Test units (BookSearchProxyModelTest, etc.)
qml/
  Main.qml     Window root, holds page Loader
  pages/
    mainPage/         MainPage + sections (Currently reading, Categories, …)
    categoryListPage/ CategoryListPage (vertical book list per category)
    bookDetailPage/   BookDetailPage + header / progress / ratings / reading-history / characters cards
  components/  Reusable UI: SurfaceCard / PressableSurface / PaddedCard / TouchTarget
                base components, plus rows, buttons, search field, nav bar,
                progress widgets, StarRating, TagPill
  theme/       Theme singleton + palettes (Pink/Blue/Yellow/Purple)
  utils/       Geometry, Styles, Format (singletons: tokens + display formatting)
db/
  db_scripts.qrc  Resource manifest for SQL scripts
  init.sql        Schema (books + side tables: genres, book_genres,
                  book_characters, reading_sessions)
  test_data.sql   Seed data (debug only)
```
