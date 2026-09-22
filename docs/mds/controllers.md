# Controllers

The QML singletons live in `src/controllers/`. All follow the same pattern:
the C++ instance is created by `AppInitializer`, registered via
`setInstance()`, and the QML factory `create()` returns the prepared instance
with `QQmlEngine::CppOwnership`.

- `BookController` — book form/list state, character model, reading-history
  model, reading-timer entry points.
- `NavigationController` — page stack and routing.
- `SettingsController` — façade for user preferences. Owns
  `LanguageModel` (and any future settings models). See
  [i18n.md](i18n.md) for the language pipeline end-to-end.
- `BookStatisticsController` — everything the statistics page shows about the
  one book the detail page has open.
- `ProfileController` — the same idea one level up: what the library as a
  whole adds up to, for the profile page.
- `BookFilterController` — the filter criteria shared by the library lists and
  the online search; see [filtering.md](filtering.md).
- `GlobalBookSearchController` — the online catalog search; see
  [online-book-search.md](online-book-search.md).

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
| `deleteReadingSession(sessionId)` | `Q_INVOKABLE` | drops one `reading_sessions` row and reloads the history model. The id comes in as a `QString` — 64-bit values do not survive the QML boundary, same reason as the timer invokables below. Idempotent: an id that matches no row is not an error. Leaves `books.pagesRead` alone — the journal is not the reading position. QML confirms first |
| `readingHistoryModel` | property (RO, `ReadingHistoryModel*`) | per-current-book journal of finished reading sessions, newest first, paged the same way as `charactersModel`. Re-reads whenever `currentBookIsbn` changes — which covers a saved session too, since `bookSaved` refreshes the list model and that reset re-emits the signal |
| `getSortFilterProxyForKind(kind)` | `Q_INVOKABLE` | proxy for a specific kind |
| `openBook(id)` | `Q_INVOKABLE` | sets `currentBookId` and emits `bookOpenRequested(id)` for the router to pick up |
| `addCustomBook(fields)` | `Q_INVOKABLE` → `bool` | stores a hand-typed book from a `QVariantMap` keyed by SQL column names (`name`, `author`, `year`, `publisher`, `totalPages`, `description`, `coverUrl`, `genres`) — the shape `BookDTO::fromMap` already reads. Trims the text, applies a staged PDF over it, then requires a title and an author (otherwise sets `errorMessage` and returns false), resolves the ISBN (below), marks the row `isCustom` and files it under `WantToRead` so it lands in a category list, then emits `bookSaved` and `openBook`s it. Backs [`AddBookPage`](../../qml/pages/addBookPage/AddBookPage.qml) |
| `deleteCurrentBook()` | `Q_INVOKABLE` → `bool` | removes the current book from the library for good, cascading to its genres, characters and reading sessions, dropping both timer caches and its stored files (`BookFileStore::removeAll`). Refuses anything but a custom book — a catalog book only ever leaves a category, since it can be found online again. Clears the selection before emitting `bookSaved`, so `currentBookData` stops resolving a row that is gone. QML confirms first, then calls `NavigationController.goBack()` |
| `stagePdf(fileUrl)` | `Q_INVOKABLE` → `QVariantMap` | parses a PDF the **add form** just picked and holds it as pending. Returns `{ok, pageCount, title, author, isbn, coverPreview}` so the form can show the overwrite instead of letting it happen silently on save. `coverPreview` is the rendered first page as a `data:image/png;base64,…` url — inline because the stored cover is named after an isbn that does not exist until the add commits, so there is no file for the form to point at; on failure `{ok: false}` and `errorMessage` is set. Nothing is written yet — the stored file is named after the isbn, which does not exist until the add commits |
| `clearStagedPdf()` | `Q_INVOKABLE` | drops the pending PDF. `AddBookPage` calls it on destruction: the controller would otherwise hold it until the next add and attach it to a different book |
| `attachPdfToCurrentBook(fileUrl)` | `Q_INVOKABLE` → `bool` | parses, stores and attaches a PDF to the book on screen, replacing one that is already there. Refused when `pdfSource` is `Server`. Metadata overrides the book's own fields **only for a custom book** — a catalog book took its fields from the catalog, and the PDF has no better claim on them |
| `removePdfFromCurrentBook()` | `Q_INVOKABLE` → `bool` | deletes the stored PDF and clears both columns. Only for `PdfSource::User`; the cover stays, since once rendered it is the book's own picture. QML confirms first |

The four PDF actions and `deleteCurrentBook` are reached from the detail page's
"⋮" overflow menu, which builds its entries from the book's state — see
[qml.md](qml.md#bookdetailpageqml). The C++ side re-checks every rule anyway:
the menu decides what to show, not what is allowed.
| `openCurrentBookPdf()` | `Q_INVOKABLE` (const) → `bool` | hands the stored file to the platform viewer through `QDesktopServices` — rendering a whole book is not this app's job |
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
| `bookOpenRequested(qint64 id)` | signal | wired in `AppInitializer` to `NavigationController::setCurrentPage(BookDetailPage)` |

`ListKind` enum (`Q_ENUM`):

```
WantToRead = 0   // status == 1 in the DB
WantToBuy  = 1   // inWishList == true
AlreadyRead= 2   // status == 3
InProgress = 3   // status == 2 (used by "Currently reading" section)
```

QML accesses values as `BookController.WantToRead`, etc.

For the `status` enum used on the detail page (`BookStatus.Finished`,
`BookStatus.InProgress`, …), see [`BookStatus`](../../src/services/dto/BookStatus.hpp)
— it's a separate Q_GADGET to avoid name clashes with `ListKind` members.

For the reading-timer phase (`ReadingPhase.Stopped` / `Running` / `Paused`),
see [`ReadingPhase`](../../src/services/dto/ReadingPhase.hpp) — also Q_GADGET,
single source of truth shared between `ReadingSessionCache` (C++) and
`ReadingProgressTimer` (QML).

### How a custom book gets its ISBN

Never by generating one. In order of precedence:

1. **The PDF** — `PdfMetadataReader` scans the page text for a labelled ISBN
   and `addCustomBook` takes it over anything typed, the same precedence the
   rest of the PDF metadata has.
2. **The form** — the optional `isbnText` field, run through
   `IsbnValidator::convert`, which validates the checksum and normalises an
   ISBN-10 to its ISBN-13 form so a hand-typed book keys the same row an
   import of it would. A typed-but-invalid ISBN is **refused**, not ignored:
   quietly dropping it would file the book under no ISBN while the user
   believes otherwise.
3. **Neither** — the book is stored with no ISBN at all and keyed by
   `BookTable::nextLocalKey()` (see [database.md](database.md#booktable)).

An ISBN already in the library is refused up front, so the duplicate surfaces
as "That book is already in your library" rather than a failed insert.

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

The signal is wired in [`AppInitializer::initModels`](../../src/core/app/AppInitializer.cpp):

```cpp
connect(_bookController, &BookController::bookOpenRequested, _contextModel,
        [this](qint64) {
          _contextModel->setCurrentPage(NavigationController::Page::BookDetailPage);
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

A [`BookCharactersModel`](../../src/models/books/details/BookCharactersModel.hpp)
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
[`ReadingSessionCache`](../../src/services/caching/ReadingSessionCache.hpp) (QSettings
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

## `BookStatisticsController`

Backs [`BookStatisticsPage`](../../qml/pages/bookStatisticsPage/BookStatisticsPage.qml).
It never picks a book itself: `AppInitializer` pushes
`BookController::currentBookIsbn` in whenever the open book changes, and
re-runs the computation on `BookController::readingJournalChanged`. Everything
is derived from that book's `reading_sessions` rows on read — nothing is
stored, and there is no cache to invalidate.

The ISBN is **not** QML-visible: the page never picks a book, and a 64-bit
ISBN written from QML would truncate anyway. The C++ setter early-returns on
an unchanged ISBN, which matters because `currentBookIsbnChanged` doubles as
a cache-invalidation ping — `BookController` re-emits it on every book-list
reset, so without the guard an unrelated edit like a wishlist toggle would
re-read the journal. The two channels then split cleanly: that signal picks
the book, `readingJournalChanged` recomputes it.

### QML-visible API

| Member | Kind | Purpose |
|--------|------|---------|
| `statistics` | property (RO, `readary::qmltypes::BookStatisticsObject` Q_GADGET) | the whole set in one value (below) |
| `hasData` | property (RO, `bool`) | whether the book has any finished session at all |
| `refresh()` | `Q_INVOKABLE` | recomputes from the journal as it stands now, and stays silent when the figures come back unchanged — the page calls this on every open, and an emit there would tear down and rebuild every chart delegate for an identical result |

`statistics` fields, as QML sees them:

| Field | Meaning |
|-------|---------|
| `sessionCount` | finished sessions for this book |
| `timedSessionCount` | how many of those carry a reading speed — the only way to tell a displayed `0` p/h from "nothing was ever timed", since both leave the three speeds at `0.0` |
| `pagesRead` | journal total, re-reads included — **not** the reading position (`books.pagesRead` is that) |
| `totalSeconds` | summed session durations |
| `averagePagesPerHour` | pages over time across the **timed** sessions — the duration-weighted mean of their speeds, not a plain mean |
| `minPagesPerHour` / `maxPagesPerHour` | slowest and fastest single session |
| `weeklyPages` | seven ints, Monday..Sunday of the **current** week; always seven, so the chart keeps a column per weekday even for a book last read months ago |
| `progressPoints` | `[{session, page}]`, oldest session first — the page reached when each session ended |

### Which sessions carry a speed

All three speeds come from the same set: the sessions that were actually
timed **and** logged an end page. A session saved the instant it started has
no duration to divide by, so it carries no speed. Neither does one whose
`pages_to` is NULL — `db/init.sql` permits that on a finished row and
`ReadingSessionDTO` reads it back as `0`, which is a missing measurement, not
zero pages; `pagesTo < pagesFrom` identifies it exactly, because the schema's
CHECK forbids every other way to get there. A timed session that really did
gain no page is **kept** — 0 p/h is a real, and the reader's slowest,
session, which is why the page needs `timedSessionCount` to render it as `0`
rather than as a dash.

Sharing one set is what keeps the card honest: the average is then the
duration-weighted mean of the per-session speeds, so it can never fall
outside `minPagesPerHour`..`maxPagesPerHour`, which is exactly what the card
prints it between. Computing the average over the whole journal instead —
`pagesRead / totalSeconds` — reads as the more obvious definition but lets
untimed sessions push it outside its own range.

`pagesRead`, `totalSeconds` and `sessionCount` stay whole-journal sums
regardless: a session that measured no speed was still time the reader
spent. The three tiles are therefore not divisible into one another once an
untimed row exists — Pages and Reading are whole-journal, Pages/h is
timed-only.

### File map

| File | Purpose |
|------|---------|
| [src/controllers/BookStatisticsController.hpp](../../src/controllers/BookStatisticsController.hpp) | Properties, singleton wiring |
| [src/controllers/BookStatisticsController.cpp](../../src/controllers/BookStatisticsController.cpp) | Journal read + recompute |
| [src/services/statistics/BookStatisticsCalculator.hpp](../../src/services/statistics/BookStatisticsCalculator.hpp) | The pure computation, testable without a database |
| [src/services/dto/BookStatisticsDTO.hpp](../../src/services/dto/BookStatisticsDTO.hpp) | The plain, moc-free result struct |
| [src/qmltypes/BookStatisticsObject.hpp](../../src/qmltypes/BookStatisticsObject.hpp) | Q_GADGET wrapper at the QML boundary |

## `ProfileController`

Backs [`ProfilePage`](../../qml/pages/profilePage/ProfilePage.qml). Where
`BookStatisticsController` describes one book, this one describes the shelf:
how many books are behind the reader, how many pages that is, and how long
the timer has run across all of them. Everything is recomputed from `books`
and `reading_sessions` on read — nothing is stored, and there is no cache to
invalidate.

`AppInitializer` re-runs it on `BookListModel::modelReset` (which fires after
any book write, since `BookController::bookSaved` reloads the list) and on
`BookController::readingJournalChanged`. The page also calls `refresh()` on
open, because it lives behind `Main.qml`'s `Loader` and is rebuilt each time
while the controller's figures are not.

### QML-visible API

| Member | Kind | Purpose |
|--------|------|---------|
| `statistics` | property (RO, `readary::qmltypes::LibraryStatisticsObject` Q_GADGET) | the whole set in one value (below) |
| `readerLevel` | property (RO, `ReaderLevel`) | the badge over the profile card, derived from `booksFinished` alone |
| `hasData` | property (RO, `bool`) | whether the library holds any book at all |
| `refresh()` | `Q_INVOKABLE` | recomputes from the library as it stands now, and stays silent when the figures come back unchanged |

`statistics` fields, as QML sees them:

| Field | Meaning |
|-------|---------|
| `booksTotal` | rows in `books` |
| `booksFinished` | `status = Finished` |
| `booksInProgress` | `status = InProgress` |
| `pagesRead` | pages behind the reader across the library (below) |
| `totalSeconds` | summed durations of every finished session, all books |
| `sessionCount` | finished sessions, all books |

`ReaderLevel` steps with `booksFinished`: `Newcomer` at 0, `Reader` from 1,
`Bookworm` from 5, `Bibliophile` from 20. The thresholds live in C++ so they
are testable; QML owns the wording, so `retranslate` reaches it.

### Why pages are counted per book, not from the journal

`BookStatisticsDTO::pagesRead` sums the journal, which is right for one book:
re-reading it really is more pages read. Across the library that sum would be
wrong twice over — it double-counts re-reads against the shelf, and it is
empty for anyone who never used the reading timer, which is most of a library
imported from a catalog. So each book contributes its own reading position
(`books.pagesRead`) instead, with two adjustments: a book marked `Finished`
counts its whole `totalPages`, because setting the status from the detail page
never writes a position; and a position past the stated length wins over it,
because an imported page count can simply be too low.

`totalSeconds` and `sessionCount` still come from the journal — that is the
only place the timer records anything.

### File map

| File | Purpose |
|------|---------|
| [src/controllers/ProfileController.hpp](../../src/controllers/ProfileController.hpp) | Properties, `ReaderLevel` thresholds, singleton wiring |
| [src/controllers/ProfileController.cpp](../../src/controllers/ProfileController.cpp) | Library read + recompute |
| [src/services/statistics/LibraryStatisticsCalculator.hpp](../../src/services/statistics/LibraryStatisticsCalculator.hpp) | The pure computation, testable without a database |
| [src/services/dto/LibraryStatisticsDTO.hpp](../../src/services/dto/LibraryStatisticsDTO.hpp) | The plain, moc-free result struct |
| [src/qmltypes/LibraryStatisticsObject.hpp](../../src/qmltypes/LibraryStatisticsObject.hpp) | Q_GADGET wrapper at the QML boundary |

## `NavigationController`

Tiny stack-based router. Holds a `QStack<Page>`; pushing a page that
already sits below the top is rejected (no duplicate pushes). Levels enforce
hierarchy — a higher-level page replaces lower-or-equal levels on push, so
pushing `MainPage` (level 1) over `CategoryListPage` (level 2) collapses
back to the root.

### QML-visible API

| Member | Kind | Purpose |
|--------|------|---------|
| `currentPage` | property (R/W, `Page`) | top of the stack |
| `currentPagePath` | property (RO, QUrl) | qrc URL of the QML for the top page |
| `goBack()` | `Q_INVOKABLE` | pops one page; no-op if stack has 1 entry |

`Page`:

```
MainPage         = 1   level 1   qrc:/qt/qml/pages/mainPage/MainPage.qml
CategoryListPage = 2   level 2   qrc:/qt/qml/pages/categoryListPage/CategoryListPage.qml
SearchPage       = 3   level 1   qrc:/qt/qml/pages/searchPage/SearchPage.qml
GoalsPage        = 4   level 1   (placeholder — falls back to MainPage)
ChallengesPage   = 5   level 1   (placeholder — falls back to MainPage)
ProfilePage      = 6   level 1   qrc:/qt/qml/pages/profilePage/ProfilePage.qml
SettingsPage     = 7   level 2   qrc:/qt/qml/pages/settingsPage/SettingsPage.qml
AddBookPage        = 8   level 3   qrc:/qt/qml/pages/addBookPage/AddBookPage.qml
BookDetailPage     = 9   level 3   qrc:/qt/qml/pages/bookDetailPage/BookDetailPage.qml
BookStatisticsPage = 10  level 4   qrc:/qt/qml/pages/bookStatisticsPage/BookStatisticsPage.qml
```

The two remaining placeholder pages (Goals/Challenges) are exposed so
`BottomNavBar` can drive `currentPage` to them, but their `pageInfo()` entry
maps to `MainPage`'s URL — they'll get real implementations later.
`SettingsPage` is level 2 under `ProfilePage`: it is reached from the profile
page's section list, so its back arrow has to land there rather than unwind to
the library. It is not a bottom-nav destination of its own.

`AddBookPage` deliberately shares level 3 with `BookDetailPage`: a successful
add ends in `BookController::openBook`, and a same-level push replaces the
form instead of stacking on top of it — so `goBack` from the new book's detail
page lands on the search page, never on a filled-in form.

`BookStatisticsPage` is the only level-4 page: it is reached from the detail
page's Statistics tile and describes the book that page has open, so its back
arrow has to land back on it rather than unwind to the list.

### Wiring in QML

Top-level `Main.qml` holds a `Loader` whose `source` is bound to
`NavigationController.currentPagePath`. Anything that wants to navigate sets
`NavigationController.currentPage = …` (or calls `goBack()`).

For book detail entry the canonical path is `BookController.openBook(id)`,
not direct `currentPage = BookDetailPage` — see the section above.

### File map

| File | Purpose |
|------|---------|
| [src/controllers/NavigationController.hpp](../../src/controllers/NavigationController.hpp) | Enum + properties |
| [src/controllers/NavigationController.cpp](../../src/controllers/NavigationController.cpp) | Stack logic, page→URL mapping |
| [qml/Main.qml](../../qml/Main.qml) | `Loader.source: NavigationController.currentPagePath` |
