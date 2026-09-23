# Database layer

SQLite via Qt SQL. Three pieces:

- `DatabaseManager` — the connection wrapper and query runner.
- `SqlQueryBuilder` — fluent builder that produces a parameterized SQL
  string and a `QVariantList` of bound values.
- `BookTable` — typed CRUD over `BookDTO`, plus side-table accessors
  (`getGenres`, `getCharacters`) for the book detail page and progress
  writers (`updatePagesRead`, `insertReadingSession`) for the reading timer.
- `ReadingSessionCache` — `QSettings`-backed per-book timer-state store
  (cross-launch persistence). Static-only, called by `BookController`.

All three live in `src/core/db/` and `src/services/storage/`. The DB layer **does not
depend on Qt Quick or QML** — it can run from tests or other non-UI code.

## `DatabaseManager`

Wraps a `QSqlDatabase` connection (named `"Readary"`).

| Method | Purpose |
|--------|---------|
| `open()` / `close()` | Open/close the underlying SQLite file |
| `runScript(qrc-or-fs path)` | Read and execute a multi-statement SQL script |
| `exec(sql)` | One-shot `QSqlQuery::exec` wrapped in a transaction |
| `select(builder)` | Run a SELECT, return `QList<QVariantMap>` (column → value) |
| `execute(builder)` | Run UPDATE/DELETE in a transaction; returns rows affected |
| `insert(builder)` | Run INSERT in a transaction; returns `lastInsertId()` |

Every write is wrapped in `begin/commit` (with `rollback` on error). Failures
are logged to `lcDb` (`bl.core.db`) and optionally written to the caller-
provided `QString *error`.

`runScript` parses statements by walking lines: a line that starts with a
non-whitespace character begins a statement; lines starting with whitespace
are continuations; `--` comments are skipped; statements end at a line whose
trimmed tail is `;`.

## `SqlQueryBuilder`

Fluent method chains; every method returns `*this`. Maintains the SQL string
in `_query` and the parameter list in `_values`.

```cpp
SqlQueryBuilder b;
b.select({"id", "name"}).from("books").where("status = ?").values({3});
auto rows = db.select(b);

SqlQueryBuilder ins;
ins.insertInto("books", {"name", "year"}).values({"X", 2024});
qint64 newId = db.insert(ins);
```

Supported clauses: `select`, `selectCount`, `insertInto`, `update`,
`from` (with optional alias), `deleteFrom`, `where`, `set`, `leftJoin`,
`on`, `orderBy`, `values` (lvalue or rvalue overload).

Values are bound via `?` placeholders (positional). The builder doesn't
escape strings — it relies on `QSqlQuery::addBindValue` in
`DatabaseManager::execPrepared`.

## `BookDTO`

Plain C++ struct mirroring the books table:

```cpp
struct BookDTO {
  qint64  isbn = 0;             // primary key; a real ISBN, or a local key
  QString name;
  QString authorName;
  int     year = 0;
  QString publisherName;
  QString description;
  QString coverUrl;
  bool    isHardcover = false;
  QString typeName;
  int     totalPages = 0;       // 0 = unknown, stored as NULL
  int     pagesRead = 0;
  double  globalRating = 0.0;   // external source (e.g. Goodreads)
  double  localRating = 0.0;    // averaged across this app's users
  int     userRating = 0;       // current user's own rating
  int     status = 0;           // mirrors BookStatus::Value
  bool    inWishList = false;
  QString language;
  bool    isCustom = false;     // added by hand, not imported from a catalog
  QString pdfPath;              // file in the app data dir, empty when none
  int     pdfSource = 0;        // mirrors PdfSource::Value
  QString workKey;              // import-time only, no column
  QStringList genres;           // side table, not part of the books row

  static BookDTO fromMap(const QVariantMap &);
  QVariantMap toMap() const;
};
```

`BookDTO` itself is a plain POD — no Q_GADGET, no moc — so the services
layer stays free of QML metadata. The QML-facing wrapper lives in a
separate `qmltypes/` layer at
[`src/qmltypes/BookDTOObject.hpp`](../../src/qmltypes/BookDTOObject.hpp):
it inherits from `BookDTO`, adds a Q_GADGET + Q_PROPERTY MEMBER aliases
for every field, and tacks on the detail-view-only `QStringList genres`.
`BookController::currentBookData` returns the wrapper so QML reads
members directly off the gadget without going through a `QVariantMap`
shim; data-layer code (BookListModel, BookTable) keeps using the bare
`BookDTO`.

`status` is plain `int` in storage; the QML-visible enum lives in
[`BookStatus`](../../src/services/dto/BookStatus.hpp) (Q_GADGET, `QML_ELEMENT`),
so QML compares as `BookStatus.Finished` instead of magic literal `3`.
Filter strategies (see [models-and-filters.md](models-and-filters.md))
still use raw int values internally.

The three rating fields are deliberately distinct, not duplicates —
see [`BookSortFilterProxyModel::lessThan`](../../src/models/books/proxy/BookSortFilterProxyModel.cpp)
for the comparison policy (year/totalPages/pagesRead as int, ratings as
double).

## `BookTable`

Typed CRUD over `BookDTO`. Composes joins so `getAllBooks()` returns each
book with its author/publisher/type names already resolved.

| Method | Purpose |
|--------|---------|
| `getAllBooks()` | Full SELECT with joins → `QList<BookDTO>` |
| `addBook(book)` | INSERT; returns the row's key — `book.isbn` when the book has one, otherwise the key `nextLocalKey()` hands out |
| `updateBook(book)` | UPDATE by id |
| `deleteBook(id)` | DELETE by id; genres, characters and sessions follow through `ON DELETE CASCADE` |
| `getGenres(bookId) const` | `QStringList` of genre names for one book |
| `getCharacters(bookId) const` | `QList<CharacterDTO>` (`id`, `name`, `role`) for one book; consumed by `BookCharactersModel` |
| `getReadingSessions(bookId) const` | `QList<ReadingSessionDTO>` of *closed* sessions (`ended_at IS NOT NULL`) for one book, newest first; consumed by `ReadingHistoryModel` |
| `getAllReadingSessions() const` | The same *closed* rows for the whole library, newest first; consumed by `ProfileController` and `ReadingStatisticsController` — both count the journal as a whole, and one query beats one per book |
| `updatePagesRead(bookId, pagesRead)` | Targeted `UPDATE books SET pagesRead = ?`; called by `BookController::updateReadingProgress` after a session ends |
| `deleteReadingSession(sessionId)` | Deletes one journal row; `books.pagesRead` is deliberately untouched. Idempotent — an id that matches nothing reports success, since only a failed statement is a failure |
| `insertReadingSession(bookId, pagesFrom, pagesTo, durationSeconds)` | Inserts one row in `reading_sessions` with `started_at = now − duration` |

`ReadingSessionDTO` (`services/ReadingSessionDTO.hpp`) carries the raw row
(`id`, `bookIsbn`, `startedAt`, `endedAt`, `pagesFrom`, `pagesTo`) and derives
`pagesRead()` / `durationSeconds()` on read — the journal stores no totals, so
there is nothing to keep in sync. Timestamps come back as TEXT and are parsed
with `Qt::ISODate`, then normalized to local time for display.

Side-table reads (`getGenres`, `getCharacters`, `getReadingSessions`) are intentionally NOT folded
into `getAllBooks()` — list views don't need them, and joining them on every
list refresh would multiply rows. `getGenres` is pulled on demand by
`BookController::currentBookData()` and assigned onto the cached `BookDTO`
per `currentBookId`. `getCharacters` is pulled by `BookCharactersModel`
(owned by `BookController`) on every `setBookId` and held in memory for
paged exposure to QML. `getReadingSessions` works the same way through
`ReadingHistoryModel`.

## Schema overview

```
authors        (id, name)
publishers     (id, name)
book_types     (id, name)
genres         (id, name)

books          (isbn, name, author,
                year, publisher,
                description, coverUrl,
                isHardcover, type,
                totalPages, pagesRead,
                globalRating, localRating, userRating,
                status, inWishList, language, isCustom)

book_genres        (book_id, genre_id)             -- M:N
book_characters    (id, book_id -> books, name, role)
reading_sessions   (id, book_id -> books,
                    started_at, ended_at,
                    pages_from, pages_to)
```

CHECK constraints enforce ranges on `status` (0..3),
`inWishList`/`isHardcover`/`isCustom` (0/1), and 0..10 on all three rating
columns. `totalPages` allows NULL for books without page count yet — and only
NULL, never 0, so `BookTable` binds the DTO's "unknown" 0 as a typed NULL on
insert and update; `pagesRead` defaults to 0.

**No ISBN is ever invented.** A hand-added book is stored under the ISBN read
out of its PDF, or the one the user typed — and under neither when it has
none, which is the normal case for a notebook or a manuscript. The `isbn`
column is still the primary key, so such a row is keyed by
`BookTable::nextLocalKey()`: `MAX(isbn) + 1` counted over keys *below* the
ISBN-13 floor (`9'780'000'000'000` — every ISBN-13 is 978/979-prefixed), so
the first one is `1`. A key that small cannot be mistaken for an ISBN, and it
can never collide with a real one imported later.

SQLite's own rowid is deliberately *not* used for this: `isbn INTEGER PRIMARY
KEY` is the rowid, and its next value is `MAX(rowid) + 1` over the whole
table — in a library holding real ISBNs that yields an ISBN-shaped number,
which is exactly what this avoids.

`isCustom` marks a book the user typed in themselves rather than imported.
It gates deletion: `BookController::deleteCurrentBook` refuses a catalog book
(it can always be found online again) and only removes a custom one.

`pdfPath` / `pdfSource` describe the book's attached PDF. The source is not
just provenance, it carries a rule — see
[`PdfSource`](../../src/services/pdf/PdfSource.hpp):

| Value | Meaning |
|-------|---------|
| `None` (0) | no PDF; `pdfPath` is NULL |
| `Server` (1) | supplied by a catalog. Openable, never replaceable or removable — the file is not the user's to change. No catalog currently populates this, so nothing writes a 1 yet; the rule is enforced from day one so that wiring one up later needs no migration. |
| `User` (2) | attached by the user. Replaceable and removable. |

The file itself never lives where the user picked it — see `BookFileStore`
below. `globalRating` /
`localRating` are REAL (averages); `userRating` is INTEGER (whole stars).

`reading_sessions` is a **journal**, not a position cache. Sessions may
overlap (re-reading same range), so:
- Current reading position = `books.pagesRead` (app-managed).
- "Total pages read incl. re-reads" = `SUM(pages_to - pages_from)`.
- "Position from log alone" = `MAX(pages_to) WHERE book_id = ?`.

The big comment at the top of [`init.sql`](../../db/init.sql) lists the
derived metrics formulas (julianday for duration, etc.).

## `BookFileStore` (files on disk)

[`BookFileStore`](../../src/services/storage/BookFileStore.cpp) owns everything a book
keeps outside the database — its PDF and the cover rendered from it — under one
root the caller supplies: `AppEnvironment::bookFilesPath()`
(`<dataPath>/books`) in the app, a `QTemporaryDir` in tests.

| Method | Purpose |
|--------|---------|
| `storePdf(isbn, sourcePath)` | Copies the file to `<root>/pdfs/<isbn>.pdf` and returns that path. Replaces an existing one, since swapping a user PDF is a supported action — and returns early when the pick already *is* the stored file, which the dialog allows and which would otherwise delete it |
| `storeCover(isbn, image)` | Writes `<root>/covers/<isbn>.png` and returns that path. Fed both by the PDF's rendered first page and by a cover the user picked by hand — a picked file is copied in, not referenced, and scaled to the same bound |
| `coverUrl(isbn)` | The file url to put in `books.coverUrl`, with a hash of the file's contents as a `?v=` query. Cover files keep a stable name, and `Image` caches by url — a re-rendered cover at an unchanged url would keep showing the previous PDF's first page. `QUrl::toLocalFile()` drops the query so the file still opens, and identical content still yields an identical url, so an unchanged cover does not invalidate a good cached image |
| `removePdf(isbn)` | Deletes the stored PDF, leaving the cover. Idempotent — nothing to delete is the end state the caller asked for |
| `removeAll(isbn)` | Both files. Called when the book itself goes away |
| `toLocalPath(fileUrl)` | `file://…` → path. A non-local URL passes through untouched: Android hands over `content://…`, which has no local path but which Qt's file engine opens directly |

**Copied in, not referenced.** The library has to keep working after the user
moves, renames or deletes the file they picked, so the pick is a copy and the
DB stores the copy's path. Everything is named after the book's key, which makes
cleanup on delete a lookup rather than a search — and means a local key handed
out again by `nextLocalKey()` would land on the previous book's files, so
`BookController::deleteCurrentBook` calls `removeAll` on the way out.

## `PdfMetadataReader` (Qt PDF)

[`PdfMetadataReader`](../../src/services/pdf/PdfMetadataReader.cpp) is what makes an
attached PDF worth attaching: one static `read()` over `QPdfDocument` (PDFium,
from the vcpkg `qtwebengine[pdf]` port) returning a
[`PdfDocumentInfo`](../../src/services/pdf/PdfDocumentInfo.hpp).

| Field | Where it comes from | Reliability |
|-------|---------------------|-------------|
| `pageCount` | the page tree | always present; `pageCount == 0` is what `isValid()` reports as a failed read |
| `isbn` | the page **text**, front matter then back matter | no PDF metadata field carries an ISBN, so it is read off the page the way a person would. 0 when the book prints none where we look, or prints it as an image (a pure scan). Bounded to 12 front and 4 back pages — a 2000-page manual is not read end to end for one number |
| `title` / `author` / `subject` | the `/Info` dictionary | frequently empty, and sometimes authoring-tool noise |
| `cover` | page one, rendered | absent only if the first page has no size |

The cover render is scaled into `kDefaultCoverSize` (300×420) with the aspect
ratio preserved — a page is never the cover slot's shape, and a stretched cover
looks broken.

Because `/Info` is so often blank, **an empty field never overrides**: see
`BookController::overrideFromPdf` in
[controllers.md](controllers.md#bookcontroller).

## `ReadingSessionCache` (QSettings)

Sits next to `BookTable` in `services/`. Stores per-book reading-timer state
(`{seconds, phase, lastSyncAt}`) in `QSettings` under group
`readingSession/<bookId>/`. Static methods, no instance state — the storage
is the file QSettings writes to (registry on Windows, `~/.config` on Linux,
plist on macOS).

| Method | Purpose |
|--------|---------|
| `save(bookId, seconds, phase)` | Writes the snapshot and refreshes `lastSyncAt = now`. Called periodically (every 5s while running) and on QML page destroy. |
| `takeState(bookId)` | Reads-and-deletes the snapshot. When the saved phase was `Running`, adds wall-clock seconds elapsed since `lastSyncAt` so a running timer keeps counting across restarts. Returns `{}` if nothing saved. |
| `clear(bookId)` | Drops the group; called on page destroy when phase is `Stopped`. |

**Why QSettings, not the SQL DB:** the cache is fast-path UI state, not a
permanent log. Sessions that complete (user hits "Save") get logged to
`reading_sessions` via `BookTable::insertReadingSession`; sessions in
progress (or paused) live only in the cache and are reset by app restart
without restore.

For QSettings to land in a stable location across debug/release/installer
builds, [`main.cpp`](../../src/main.cpp) sets
`QGuiApplication::setOrganizationName/Domain/setApplicationName` before any
QSettings instance is constructed.

The QML-visible enum that mirrors the integer `phase` column is
[`ReadingPhase`](../../src/services/dto/ReadingPhase.hpp) (Q_GADGET):

```cpp
enum Value {
    Stopped = 0,
    Running = 1,
    Paused  = 2,
};
```

C++ uses `ReadingPhase::Running` directly, QML writes
`ReadingPhase.Running` — single source of truth.

## Schema and seed data

- `db/init.sql` — schema. Uses `CREATE TABLE IF NOT EXISTS`, so an added
  column never reaches an existing DB file through the `CREATE` — there is no
  migration runner. Each such column therefore gets a plain `ALTER TABLE …
  ADD COLUMN` after the create (see `isCustom`); SQLite has no
  `IF NOT EXISTS` for that, so once the column is in place the statement fails
  harmlessly and logs one `duplicate column name` warning per launch under
  `readary.core.db`.
- `db/test_data.sql` — seed rows used in **debug builds only**, executed
  after `init.sql` from `AppInitializer::initDatabase`. Uses
  `INSERT OR REPLACE INTO books`; child rows in `book_genres` /
  `book_characters` / `reading_sessions` get cleaned by `ON DELETE CASCADE`
  when the parent book row is replaced, then re-inserted.

Cover URLs in seed data point to Open Library Covers API by ISBN; missing
ISBNs fall back to the QML 📖 placeholder via `Image.status` check.

Both scripts are bundled via `db/db_scripts.qrc` under prefix `/db`. See
[build-and-resources.md](build-and-resources.md#qrc-resources) for the qrc
wiring and the `Q_INIT_RESOURCE` quirk in `main.cpp`.

## File map

| File | Purpose |
|------|---------|
| [src/core/db/DatabaseManager.hpp](../../src/core/db/DatabaseManager.hpp) / [.cpp](../../src/core/db/DatabaseManager.cpp) | Connection, transactions, script runner |
| [src/core/db/SqlQueryBuilder.hpp](../../src/core/db/SqlQueryBuilder.hpp) / [.cpp](../../src/core/db/SqlQueryBuilder.cpp) | Fluent query builder |
| [src/services/dto/BookDTO.hpp](../../src/services/dto/BookDTO.hpp) / [.cpp](../../src/services/dto/BookDTO.cpp) | DTO, `toMap`/`fromMap` |
| [src/services/dto/BookStatus.hpp](../../src/services/dto/BookStatus.hpp) | Q_GADGET enum-namespace mirroring `status` for QML |
| [src/services/storage/BookTable.hpp](../../src/services/storage/BookTable.hpp) / [.cpp](../../src/services/storage/BookTable.cpp) | CRUD + genres/characters readers + reading-progress writers |
| [src/services/dto/ReadingPhase.hpp](../../src/services/dto/ReadingPhase.hpp) | Q_GADGET enum-namespace shared by C++ cache and QML timer |
| [src/services/caching/ReadingSessionCache.hpp](../../src/services/caching/ReadingSessionCache.hpp) / [.cpp](../../src/services/caching/ReadingSessionCache.cpp) | Per-book reading-timer state via QSettings (cross-launch) |
| [db/init.sql](../../db/init.sql) | Schema |
| [db/test_data.sql](../../db/test_data.sql) | Seed (debug only) |
| [db/db_scripts.qrc](../../db/db_scripts.qrc) | Resource manifest |
