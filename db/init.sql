PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS genres (
  id   INTEGER PRIMARY KEY AUTOINCREMENT,
  name TEXT    NOT NULL UNIQUE
);

-- status: 0 = NONE, 1 = WantToRead, 2 = InProgress, 3 = Finished
CREATE TABLE IF NOT EXISTS books (
  isbn         INTEGER PRIMARY KEY,
  name         TEXT    NOT NULL,
  author       TEXT    NOT NULL,
  year         INTEGER DEFAULT NULL,
  publisher    TEXT    DEFAULT NULL,
  description  TEXT    DEFAULT NULL,
  coverUrl     TEXT    DEFAULT NULL,
  isHardcover  INTEGER NOT NULL DEFAULT 0 CHECK (isHardcover IN (0, 1)),
  type         TEXT    DEFAULT NULL,
  totalPages   INTEGER DEFAULT NULL CHECK (totalPages IS NULL OR totalPages > 0),
  pagesRead    INTEGER NOT NULL DEFAULT 0 CHECK (pagesRead >= 0),
  globalRating REAL    DEFAULT NULL CHECK (globalRating IS NULL OR (globalRating >= 0 AND globalRating <= 10)),
  localRating  REAL    DEFAULT NULL CHECK (localRating  IS NULL OR (localRating  >= 0 AND localRating  <= 10)),
  userRating   INTEGER DEFAULT NULL CHECK (userRating   IS NULL OR (userRating   >= 0 AND userRating   <= 10)),
  status       INTEGER NOT NULL DEFAULT 0 CHECK (status IN (0, 1, 2, 3)),
  inWishList   INTEGER NOT NULL DEFAULT 0 CHECK (inWishList IN (0, 1)),
  language     TEXT    DEFAULT NULL
);

CREATE TABLE IF NOT EXISTS book_genres (
  book_isbn INTEGER NOT NULL REFERENCES books(isbn) ON UPDATE CASCADE ON DELETE CASCADE,
  genre_id  INTEGER NOT NULL REFERENCES genres(id)  ON UPDATE CASCADE ON DELETE CASCADE,
  PRIMARY KEY (book_isbn, genre_id)
);

CREATE TABLE IF NOT EXISTS book_characters (
  id        INTEGER PRIMARY KEY AUTOINCREMENT,
  book_isbn INTEGER NOT NULL REFERENCES books(isbn) ON UPDATE CASCADE ON DELETE CASCADE,
  name      TEXT    NOT NULL,
  role      TEXT    DEFAULT NULL
);

-- Reading session log. Per-book activity entries:
--   started_at / ended_at — ISO8601 timestamps; ended_at IS NULL while a session is in progress.
--   pages_from / pages_to — page numbers at session start / end.
-- Source of truth:
--   books.pagesRead is THE current reading position (updated by the application).
--   reading_sessions is a journal — overlapping ranges (re-reading) are allowed,
--   so SUM(pages_to - pages_from) is "total pages read incl. re-reads", not the position.
--   To find the current position from the log only: MAX(pages_to) WHERE book_isbn = ?.
-- Derived metrics (do NOT denormalize):
--   pages read in session     = pages_to - pages_from
--   duration of session       = julianday(ended_at) - julianday(started_at)  (in days; * 86400 for seconds)
--   when book first started   = MIN(started_at) WHERE book_isbn = ?
--   when book finished        = MAX(ended_at)   WHERE book_isbn = ? AND books.status = 3
CREATE TABLE IF NOT EXISTS reading_sessions (
  id         INTEGER PRIMARY KEY AUTOINCREMENT,
  book_isbn  INTEGER NOT NULL REFERENCES books(isbn) ON UPDATE CASCADE ON DELETE CASCADE,
  started_at TEXT    NOT NULL,
  ended_at   TEXT    DEFAULT NULL,
  pages_from INTEGER NOT NULL CHECK (pages_from >= 0),
  pages_to   INTEGER DEFAULT NULL,
  CHECK (pages_to IS NULL OR pages_to >= pages_from),
  CHECK (ended_at IS NULL OR ended_at >= started_at)
);

CREATE INDEX IF NOT EXISTS ix_books_name              ON books(name);
CREATE INDEX IF NOT EXISTS ix_books_author            ON books(author);
CREATE INDEX IF NOT EXISTS ix_books_publisher         ON books(publisher);
CREATE INDEX IF NOT EXISTS ix_books_type              ON books(type);
CREATE INDEX IF NOT EXISTS ix_books_status            ON books(status);
CREATE INDEX IF NOT EXISTS ix_book_genres_book_isbn      ON book_genres(book_isbn);
CREATE INDEX IF NOT EXISTS ix_book_genres_genre_id       ON book_genres(genre_id);
CREATE INDEX IF NOT EXISTS ix_book_characters_book_isbn  ON book_characters(book_isbn);
CREATE INDEX IF NOT EXISTS ix_reading_sessions_book_isbn ON reading_sessions(book_isbn);
CREATE INDEX IF NOT EXISTS ix_reading_sessions_started   ON reading_sessions(started_at);
