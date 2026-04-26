PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS publishers (
  id   INTEGER PRIMARY KEY AUTOINCREMENT,
  name TEXT    NOT NULL
);

CREATE TABLE IF NOT EXISTS authors (
  id   INTEGER PRIMARY KEY AUTOINCREMENT,
  name TEXT    NOT NULL
);

-- basic, delux, limited, etc
CREATE TABLE IF NOT EXISTS book_types (
  id   INTEGER PRIMARY KEY AUTOINCREMENT,
  name TEXT    NOT NULL UNIQUE
);

-- status: 0 = NONE, 1 = WantToRead, 2 = InProgress, 3 = Finished
CREATE TABLE IF NOT EXISTS books (
  id           INTEGER PRIMARY KEY AUTOINCREMENT,
  name         TEXT    NOT NULL,
  author       INTEGER NOT NULL REFERENCES authors(id)     ON UPDATE CASCADE ON DELETE RESTRICT,
  year         INTEGER DEFAULT NULL,
  publisher    INTEGER          REFERENCES publishers(id)  ON UPDATE CASCADE ON DELETE SET NULL,
  description  TEXT    DEFAULT NULL,
  isHardcover  INTEGER NOT NULL DEFAULT 0 CHECK (isHardcover IN (0, 1)),
  type         INTEGER          REFERENCES book_types(id)  ON UPDATE CASCADE ON DELETE SET NULL,
  globalRating INTEGER DEFAULT NULL,
  localRating  INTEGER DEFAULT NULL,
  userRating   INTEGER DEFAULT NULL,
  status       INTEGER NOT NULL DEFAULT 0 CHECK (status IN (0, 1, 2, 3)),
  inWishList   INTEGER NOT NULL DEFAULT 0 CHECK (inWishList IN (0, 1))
);

CREATE INDEX IF NOT EXISTS ix_books_name      ON books(name);
CREATE INDEX IF NOT EXISTS ix_books_author    ON books(author);
CREATE INDEX IF NOT EXISTS ix_books_publisher ON books(publisher);
CREATE INDEX IF NOT EXISTS ix_books_type      ON books(type);
CREATE INDEX IF NOT EXISTS ix_books_status    ON books(status);
