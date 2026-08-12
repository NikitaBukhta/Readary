#include "BookTable.hpp"

#include "core/SqlQueryBuilder.hpp"

#include <QDateTime>
#include <QLoggingCategory>

using Qt::StringLiterals::operator""_s;

namespace {
Q_LOGGING_CATEGORY(lcBookTable, "readary.services.books")
}

namespace readary::services {

BookTable::BookTable(std::shared_ptr<core::DatabaseManager> db) : _db{std::move(db)} {}

QList<BookDTO> BookTable::getAllBooks() {
  core::SqlQueryBuilder query;
  QString error;

  query
      .select({
          u"b.isbn"_s,
          u"b.name"_s,
          u"b.author"_s,
          u"b.year"_s,
          u"b.publisher"_s,
          u"b.description"_s,
          u"b.coverUrl"_s,
          u"b.isHardcover"_s,
          u"b.type"_s,
          u"b.totalPages"_s,
          u"b.pagesRead"_s,
          u"b.globalRating"_s,
          u"b.localRating"_s,
          u"b.userRating"_s,
          u"b.status"_s,
          u"b.inWishList"_s,
          u"b.language"_s,
      })
      .from(kTableName, u"b"_s);

  auto data = _db->select(query, &error);
  const QHash<qint64, QStringList> genresByBook = getGenresByBook();

  QList<BookDTO> result;
  result.reserve(data.size());
  for (const auto &row : std::as_const(data)) {
    BookDTO book = BookDTO::fromMap(row);
    book.genres = genresByBook.value(book.isbn);
    result.emplaceBack(std::move(book));
  }

  qCInfo(lcBookTable) << "Loaded" << result.size() << "books";
  return result;
}

QHash<qint64, QStringList> BookTable::getGenresByBook() const {
  core::SqlQueryBuilder query;
  QString error;

  query.select({u"bg.book_isbn"_s, u"g.name"_s})
      .from(u"book_genres"_s, u"bg"_s)
      .leftJoin(u"genres"_s, u"g"_s)
      .on(u"g.id = bg.genre_id"_s)
      .orderBy(u"g.name"_s);

  auto rows = _db->select(query, &error);
  if (!error.isEmpty()) {
    qCWarning(lcBookTable) << "Failed to load genres:" << error;
  }

  QHash<qint64, QStringList> result;
  for (const auto &row : std::as_const(rows)) {
    const QString name = row.value(u"name"_s).toString();
    if (name.isEmpty()) {
      continue;
    }

    const qint64 isbn = row.value(u"book_isbn"_s).toLongLong();
    auto it = result.find(isbn);
    if (it == result.end()) {
      it = result.insert(isbn, {});
    }
    it->append(name);
  }
  return result;
}

bool BookTable::setGenres(qint64 bookIsbn, const QStringList &genres) {
  core::SqlQueryBuilder clearQuery;
  QString error;

  // Replace rather than merge: the caller owns the whole list.
  clearQuery.deleteFrom(u"book_genres"_s).where(u"book_isbn = ?"_s).values({bookIsbn});
  if (_db->execute(clearQuery, &error) < 0) {
    qCWarning(lcBookTable) << "Failed to clear genres for book isbn:" << bookIsbn << "error:" << error;
    return false;
  }

  for (const QString &rawName : genres) {
    const QString name = rawName.trimmed();
    if (name.isEmpty()) {
      continue;
    }

    core::SqlQueryBuilder addGenre;
    addGenre.insertOrIgnoreInto(u"genres"_s, {u"name"_s}).values({name});
    _db->insert(addGenre, &error); // already-present names return 0, which is fine

    core::SqlQueryBuilder findGenre;
    findGenre.select({u"id"_s}).from(u"genres"_s).where(u"name = ?"_s).values({name});
    const auto rows = _db->select(findGenre, &error);
    if (rows.isEmpty()) {
      qCWarning(lcBookTable) << "Genre lookup failed after insert:" << name << "error:" << error;
      continue;
    }

    core::SqlQueryBuilder link;
    link.insertOrIgnoreInto(u"book_genres"_s, {u"book_isbn"_s, u"genre_id"_s})
        .values({bookIsbn, rows.first().value(u"id"_s).toLongLong()});
    _db->insert(link, &error);
  }

  qCInfo(lcBookTable) << "Set" << genres.size() << "genre(s) for book isbn:" << bookIsbn;
  return true;
}

qint64 BookTable::addBook(const BookDTO &book) {
  core::SqlQueryBuilder query;
  QString error;

  // isbn is the primary key and is supplied by the caller (not autoincremented).
  query
      .insertInto(kTableName,
                  {u"isbn"_s, u"name"_s, u"author"_s, u"year"_s, u"publisher"_s, u"description"_s, u"coverUrl"_s,
                   u"isHardcover"_s, u"type"_s, u"totalPages"_s, u"pagesRead"_s, u"globalRating"_s, u"localRating"_s,
                   u"userRating"_s, u"status"_s, u"inWishList"_s, u"language"_s})
      .values({book.isbn, book.name, book.authorName, book.year, book.publisherName, book.description, book.coverUrl,
               book.isHardcover, book.typeName, book.totalPages, book.pagesRead, book.globalRating, book.localRating,
               book.userRating, book.status, book.inWishList, book.language});

  const qint64 inserted = _db->insert(query, &error);
  if (inserted <= 0) {
    qCWarning(lcBookTable) << "Failed to add book:" << book.name << "error:" << error;
    return 0;
  }

  qCInfo(lcBookTable) << "Added book isbn:" << book.isbn << "name:" << book.name;

  if (!book.genres.isEmpty()) {
    setGenres(book.isbn, book.genres);
  }

  return book.isbn;
}

bool BookTable::updateBook(const BookDTO &book) {
  core::SqlQueryBuilder query;
  QString error;

  query.update(kTableName)
      .set({u"name"_s, u"author"_s, u"year"_s, u"publisher"_s, u"description"_s, u"coverUrl"_s, u"isHardcover"_s,
            u"type"_s, u"totalPages"_s, u"pagesRead"_s, u"globalRating"_s, u"localRating"_s, u"userRating"_s,
            u"status"_s, u"inWishList"_s, u"language"_s})
      .where(u"isbn = ?"_s)
      .values({book.name, book.authorName, book.year, book.publisherName, book.description, book.coverUrl,
               book.isHardcover, book.typeName, book.totalPages, book.pagesRead, book.globalRating, book.localRating,
               book.userRating, book.status, book.inWishList, book.language, book.isbn});

  const int affected = _db->execute(query, &error);

  if (affected > 0) {
    qCInfo(lcBookTable) << "Updated book isbn:" << book.isbn << "name:" << book.name;
    if (!book.genres.isEmpty()) {
      setGenres(book.isbn, book.genres);
    }
    return true;
  }

  if (affected == 0) {
    qCWarning(lcBookTable) << "No book found with isbn:" << book.isbn;
  } else {
    qCWarning(lcBookTable) << "Failed to update book isbn:" << book.isbn << "error:" << error;
  }
  return false;
}

bool BookTable::deleteBook(qint64 isbn) {
  core::SqlQueryBuilder query;
  QString error;

  query.deleteFrom(kTableName).where(u"isbn = ?"_s).values({isbn});

  const int affected = _db->execute(query, &error);

  if (affected > 0) {
    qCInfo(lcBookTable) << "Deleted book isbn:" << isbn;
    return true;
  }

  if (affected == 0) {
    qCWarning(lcBookTable) << "No book found with isbn:" << isbn;
  } else {
    qCWarning(lcBookTable) << "Failed to delete book isbn:" << isbn << "error:" << error;
  }
  return false;
}

QStringList BookTable::getGenres(qint64 bookIsbn) const {
  core::SqlQueryBuilder query;
  QString error;

  query.select({u"g.name"_s})
      .from(u"book_genres"_s, u"bg"_s)
      .leftJoin(u"genres"_s, u"g"_s)
      .on(u"g.id = bg.genre_id"_s)
      .where(u"bg.book_isbn = ?"_s)
      .orderBy(u"g.name"_s)
      .values({bookIsbn});

  auto rows = _db->select(query, &error);
  if (!error.isEmpty()) {
    qCWarning(lcBookTable) << "Failed to load genres for book isbn:" << bookIsbn << "error:" << error;
  }

  QStringList result;
  result.reserve(rows.size());
  for (const auto &row : std::as_const(rows)) {
    result.append(row.value(u"name"_s).toString());
  }
  return result;
}

QList<CharacterDTO> BookTable::getCharacters(qint64 bookIsbn) const {
  core::SqlQueryBuilder query;
  QString error;

  query.select({u"id"_s, u"name"_s, u"role"_s})
      .from(u"book_characters"_s)
      .where(u"book_isbn = ?"_s)
      .orderBy(u"id"_s)
      .values({bookIsbn});

  auto rows = _db->select(query, &error);
  if (!error.isEmpty()) {
    qCWarning(lcBookTable) << "Failed to load characters for book isbn:" << bookIsbn << "error:" << error;
  }

  QList<CharacterDTO> result;
  result.reserve(rows.size());
  for (const auto &row : std::as_const(rows)) {
    result.emplaceBack(CharacterDTO::fromMap(row));
  }
  return result;
}

bool BookTable::updatePagesRead(qint64 bookIsbn, int pagesRead) {
  core::SqlQueryBuilder query;
  QString error;

  query.update(kTableName).set({u"pagesRead"_s}).where(u"isbn = ?"_s).values({pagesRead, bookIsbn});
  const int affected = _db->execute(query, &error);

  if (affected > 0) {
    qCInfo(lcBookTable) << "Updated pagesRead — book isbn:" << bookIsbn << "value:" << pagesRead;
    return true;
  }
  if (affected == 0) {
    qCWarning(lcBookTable) << "updatePagesRead: no book found with isbn:" << bookIsbn;
  } else {
    qCWarning(lcBookTable) << "updatePagesRead failed for book isbn:" << bookIsbn << "error:" << error;
  }
  return false;
}

qint64 BookTable::insertReadingSession(qint64 bookIsbn, int pagesFrom, int pagesTo, int durationSeconds) {
  core::SqlQueryBuilder query;
  QString error;

  const QDateTime endedAt = QDateTime::currentDateTimeUtc();
  const QDateTime startedAt = endedAt.addSecs(-durationSeconds);

  query
      .insertInto(u"reading_sessions"_s,
                  {u"book_isbn"_s, u"started_at"_s, u"ended_at"_s, u"pages_from"_s, u"pages_to"_s})
      .values({bookIsbn, startedAt.toString(Qt::ISODate), endedAt.toString(Qt::ISODate), pagesFrom, pagesTo});

  const qint64 id = _db->insert(query, &error);
  if (id > 0) {
    qCInfo(lcBookTable) << "Inserted session — book isbn:" << bookIsbn << "duration(s):" << durationSeconds;
  } else {
    qCWarning(lcBookTable) << "insertReadingSession failed for book isbn:" << bookIsbn << "error:" << error;
  }
  return id;
}

} // namespace readary::services
