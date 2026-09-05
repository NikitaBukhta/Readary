---
name: db-change
description: Change Readary's SQLite schema safely — edit init.sql and test_data.sql, mirror the change through the DTO, BookTable and SqlQueryBuilder call sites, handle the fact that there is no migration runner (existing dev DBs keep the old shape), and update docs/mds/database.md. Use when adding or altering a table, column, constraint or query.
---

# Changing the database

Layout: [`db/init.sql`](../../../db/init.sql) (schema, always run at startup),
[`db/test_data.sql`](../../../db/test_data.sql) (debug builds only), both
bundled through `db/db_scripts.qrc`. Code: `DatabaseManager` +
`SqlQueryBuilder` in `src/core/`, typed CRUD in `src/services/BookTable.*`.
Design notes: [`docs/mds/database.md`](../../../docs/mds/database.md).

## ⚠️ There is no migration runner

`init.sql` is pure `CREATE TABLE IF NOT EXISTS`. On a machine that already has
a database file, **a new column will not appear** — the `CREATE` is skipped
and every query against the new column fails at runtime, not at build time.

So when you add or change a column, do both:

1. Update the `CREATE TABLE` in `init.sql` (for fresh installs), **and**
2. Add an idempotent `ALTER TABLE` after it for existing files, e.g.
   ```sql
   ALTER TABLE books ADD COLUMN subtitle TEXT DEFAULT NULL;
   ```
   `runScript` executes statements in order and logs failures to `lcDb`; an
   `ALTER` that has already been applied errors harmlessly, but if you want it
   clean, guard it the way the surrounding script does and say so in a comment.

If you decide instead to have the user drop their local DB, **tell them
explicitly** — it lives at
`%APPDATA%/DarieszzBooks/Readary/readary.db` (`QStandardPaths::AppDataLocation`
+ `readary.db`) and deleting it loses their library.

## The checklist

1. **`db/init.sql`** — table/column/constraint. Keep the existing style:
   `IF NOT EXISTS`, explicit `CHECK` constraints for enums and ranges,
   `REFERENCES … ON UPDATE CASCADE ON DELETE CASCADE` for child tables, and a
   comment above any integer-coded enum column mapping the values.
2. **`db/test_data.sql`** — seed rows for the new shape, or existing INSERTs
   break on a `NOT NULL` addition. Debug builds run it every launch.
3. **The DTO** (`src/services/BookDTO.hpp`, `ReadingSessionDTO.hpp`, …) — add
   the field with an in-class initialiser (`QString subtitle;` /
   `qint64 id = 0;`).
4. **`BookTable`** — the read mapper, the insert/update column lists, and any
   side-table accessor (`getGenres`, `getCharacters`) or progress writer
   (`updatePagesRead`, `insertReadingSession`). Every column list is written by
   hand; missing one is a silent data loss, not a compile error.
5. **`SqlQueryBuilder` call sites** — parameters bind positionally via `?`, so
   the `values({...})` order must match the column order exactly. Never
   interpolate a value into the SQL string.
6. **Models/controllers** — a new role in the model's `roleNames()` if QML must
   see it, exposed through a controller.
7. **Enum columns** — if the integer coding mirrors a C++ enum
   (`BookStatus`, `ReadingPhase`), update both sides and the `CHECK (… IN (…))`.
8. **Tests** — `src/tests/services/` covers table access; add or extend one
   (**`add-test`** skill).
9. **[`docs/mds/database.md`](../../../docs/mds/database.md)** — the schema
   tables there are the reference; update them (**`sync-docs`** skill).

## Verifying for real

The schema only takes effect at app startup, so a green build proves nothing:

```bash
python bootstrap.py compile        # full gate
python bootstrap.py test
python bootstrap.py run            # watch the `readary.core.db` log category
```

Inspect the live file when a query misbehaves:

```bash
python -c "import sqlite3,os;d=sqlite3.connect(os.path.expandvars(r'%APPDATA%/DarieszzBooks/Readary/readary.db'));print(d.execute('PRAGMA table_info(books)').fetchall())"
```

Never write to the user's database from a test — tests use their own
connection/fixtures.
