# Database layer

SQLite via Qt SQL. Three pieces:

- `DatabaseManager` — the connection wrapper and query runner.
- `SqlQueryBuilder` — fluent builder that produces a parameterized SQL
  string and a `QVariantList` of bound values.
- `BookTable` — typed CRUD over `BookDTO`, plus side-table accessors
  (`getGenres`, `getCharacters`) for the book detail page.

All three live in `src/core/` and `src/services/`. The DB layer **does not
depend on Qt Quick or QML** — it can run from tests or other non-UI code.

## `DatabaseManager`

Wraps a `QSqlDatabase` connection (named `"BeeLibrary"`).

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
  qint64  id = 0;
  QString name;
  qint64  authorId = 0;
  QString authorName;       // joined from authors
  int     year = 0;
  qint64  publisherId = 0;
  QString publisherName;    // joined from publishers
  QString description;
  QString coverUrl;
  bool    isHardcover = false;
  qint64  typeId = 0;
  QString typeName;         // joined from book_types
  int     totalPages = 0;
  int     pagesRead = 0;
  double  globalRating = 0.0;   // external source (e.g. Goodreads)
  double  localRating = 0.0;    // averaged across this app's users
  int     userRating = 0;       // current user's own rating
  int     status = 0;           // mirrors BookStatus::Value
  bool    inWishList = false;

  QVariantMap toMap() const;
  static BookDTO fromMap(const QVariantMap &);
};
```

`status` is plain `int` in storage; the QML-visible enum lives in
[`BookStatus`](../../src/services/BookStatus.hpp) (Q_GADGET, `QML_ELEMENT`),
so QML compares as `BookStatus.Finished` instead of magic literal `3`.
Filter strategies (see [models-and-filters.md](models-and-filters.md))
still use raw int values internally.

The three rating fields are deliberately distinct, not duplicates —
see [`BookSortFilterProxyModel::lessThan`](../../src/models/BookSortFilterProxyModel.cpp)
for the comparison policy (year/totalPages/pagesRead as int, ratings as
double).

## `BookTable`

Typed CRUD over `BookDTO`. Composes joins so `getAllBooks()` returns each
book with its author/publisher/type names already resolved.

| Method | Purpose |
|--------|---------|
| `getAllBooks()` | Full SELECT with joins → `QList<BookDTO>` |
| `addBook(book)` | INSERT, returns the new id |
| `updateBook(book)` | UPDATE by id |
| `deleteBook(id)` | DELETE by id |
| `getGenres(bookId) const` | `QStringList` of genre names for one book |
| `getCharacters(bookId) const` | `QVariantList` of `{id, name, role}` maps |

Side-table reads (`getGenres`, `getCharacters`) are intentionally NOT folded
into `getAllBooks()` — list views don't need them, and joining them on every
list refresh would multiply rows. The detail page picks them up via
`BookController::currentBookData()`, which calls these methods on demand and
caches the merged map per `currentBookId`.

## Schema overview

```
authors        (id, name)
publishers     (id, name)
book_types     (id, name)
genres         (id, name)

books          (id, name, author -> authors,
                year, publisher -> publishers,
                description, coverUrl,
                isHardcover, type -> book_types,
                totalPages, pagesRead,
                globalRating, localRating, userRating,
                status, inWishList)

book_genres        (book_id, genre_id)             -- M:N
book_characters    (id, book_id -> books, name, role)
reading_sessions   (id, book_id -> books,
                    started_at, ended_at,
                    pages_from, pages_to)
```

CHECK constraints enforce ranges on `status` (0..3), `inWishList`/`isHardcover`
(0/1), and 0..10 on all three rating columns. `totalPages` allows NULL for
books without page count yet; `pagesRead` defaults to 0. `globalRating` /
`localRating` are REAL (averages); `userRating` is INTEGER (whole stars).

`reading_sessions` is a **journal**, not a position cache. Sessions may
overlap (re-reading same range), so:
- Current reading position = `books.pagesRead` (app-managed).
- "Total pages read incl. re-reads" = `SUM(pages_to - pages_from)`.
- "Position from log alone" = `MAX(pages_to) WHERE book_id = ?`.

The big comment at the top of [`init.sql`](../../db/init.sql) lists the
derived metrics formulas (julianday for duration, etc.).

## Schema and seed data

- `db/init.sql` — schema. Uses `CREATE TABLE IF NOT EXISTS`, so changing a
  column requires deleting the existing DB file (no migration system yet).
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
| [src/core/DatabaseManager.hpp](../../src/core/DatabaseManager.hpp) / [.cpp](../../src/core/DatabaseManager.cpp) | Connection, transactions, script runner |
| [src/core/SqlQueryBuilder.hpp](../../src/core/SqlQueryBuilder.hpp) / [.cpp](../../src/core/SqlQueryBuilder.cpp) | Fluent query builder |
| [src/services/BookDTO.hpp](../../src/services/BookDTO.hpp) / [.cpp](../../src/services/BookDTO.cpp) | DTO, `toMap`/`fromMap` |
| [src/services/BookStatus.hpp](../../src/services/BookStatus.hpp) | Q_GADGET enum-namespace mirroring `status` for QML |
| [src/services/BookTable.hpp](../../src/services/BookTable.hpp) / [.cpp](../../src/services/BookTable.cpp) | CRUD + genres/characters readers |
| [db/init.sql](../../db/init.sql) | Schema |
| [db/test_data.sql](../../db/test_data.sql) | Seed (debug only) |
| [db/db_scripts.qrc](../../db/db_scripts.qrc) | Resource manifest |
