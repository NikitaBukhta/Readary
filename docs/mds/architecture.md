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
controllers. Two Q_GADGET enum-namespaces — [`BookStatus`](../../src/services/dto/BookStatus.hpp)
and [`ReadingPhase`](../../src/services/dto/ReadingPhase.hpp) — are also QML-visible
through `QML_ELEMENT`, but only as type registrations (no instances).

## Object lifecycle

`AppInitializer` (in `src/core/app/`) wires everything up at startup:

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

Each top-level folder under `src/` is one layer and one namespace
(`readary::core`, `::services`, `::models`, `::controllers`, `::api`,
`::qmltypes`, `::utils`). Sub-folders inside a layer group files by role and
do **not** add namespace levels.

```
src/
  main.cpp
  core/
    app/       AppEnvironment (paths, file logger), AppInitializer (composition root)
    db/        DatabaseManager, SqlQueryBuilder
  services/
    dto/       BookDTO, CharacterDTO, ReadingSessionDTO, BookStatisticsDTO + the BookStatus / ReadingPhase enums
    storage/   BookTable (CRUD over BookDTO), BookFileStore (per-book files on disk)
    caching/   SearchCache, ReadingProgressCache, ReadingSessionCache
    pdf/       PdfMetadataReader, PdfDocumentInfo, PdfSource (Qt PDF)
    filtering/ BookFilterCriteria (the multi-criteria value object)
    emoji/     EmojiResolver (emoji char → vendored Twemoji SVG)
    statistics/ BookStatisticsCalculator (reading journal → per-book figures)
  qmltypes/    BookDTOObject, BookStatisticsObject (Q_GADGET wrappers over the DTOs,
               exposed by BookController / BookStatisticsController to QML)
  models/
    books/
      list/    BookListModelBase, BookListModel, GlobalBookSearchListModel
      proxy/   BookSortFilterProxyModel, BookSearchProxyModel, BookCriteriaFilterProxyModel
      filters/ BookFilterStrategy + 4 concrete strategies
      details/ BookCharactersModel, ReadingHistoryModel
    settings/  LanguageModel, FontModel
  controllers/ BookController, BookFilterController, BookStatisticsController,
               GlobalBookSearchController, NavigationController, SettingsController
  api/
    bookSearch/ IBookSearchAPI + OpenLibrary / Google Books clients + composite
    translate/  ITranslator + GoogleTranslator, LanguageDetector, LanguageConverter
  utils/       IsbnValidator
  tests/       Qt Test units, mirroring the layout above (+ support/ helpers)
qml/
  Main.qml     Window root, holds page Loader
  pages/
    mainPage/         MainPage + sections (Currently reading, Categories, …)
    categoryListPage/ CategoryListPage (vertical book list per category)
    bookDetailPage/   BookDetailPage + header / progress / ratings / reading-history / characters cards
    addBookPage/      AddBookPage + cover, pdf and genre pickers for a hand-added book
    searchPage/       SearchPage (online catalog search + import)
    settingsPage/     SettingsPage
  components/  Reusable UI, grouped by role — see [qml.md](qml.md):
    base/ buttons/ input/ display/ feedback/ lists/ navigation/ overlays/
  theme/       Theme singleton + palettes (Pink/Blue/Yellow/Purple)
  utils/       Geometry, Styles, Format (singletons: tokens + display formatting)
db/
  db_scripts.qrc  Resource manifest for SQL scripts
  init.sql        Schema (books + side tables: genres, book_genres,
                  book_characters, reading_sessions)
  test_data.sql   Seed data (debug only)
docs/
  mds/            These developer docs
  diagrams/       ER + use-case diagrams (drawio)
  requirements/   Product requirements PDF and its generator scripts
```

## Includes

Every header is included by its path below `src/`:

```cpp
#include "services/dto/BookDTO.hpp"
#include "models/books/proxy/BookSearchProxyModel.hpp"
```

`src` is the target's public include root, so the layer a header belongs to is
visible at the include site and two layers can never shadow each other's
basenames. CMake additionally puts each individual header folder on the
*private* include path — moc records only a header's basename, so the generated
`qmltyperegistrations.cpp` emits `#include <BookController.hpp>` for every
QML-exposed type and those bare names still have to resolve. That list is
derived from the source glob, so a new sub-folder needs no CMake edit.
