#include "BookTable.hpp"
#include "core/SqlQueryBuilder.hpp"

#include <QDateTime>
#include <QLoggingCategory>

namespace {
Q_LOGGING_CATEGORY(lcBookTable, "readary.services.books")
}

namespace readary::services {

const QString BookTable::kTableName = QStringLiteral("books");

BookTable::BookTable(std::shared_ptr<core::DatabaseManager> db) : _db{std::move(db)} {}

QList<BookDTO> BookTable::getAllBooks() {
  core::SqlQueryBuilder query;
  QString error;

  query
      .select({
          "b.isbn",
          "b.name",
          "b.author",
          "b.year",
          "b.publisher",
          "b.description",
          "b.coverUrl",
          "b.isHardcover",
          "b.type",
          "b.totalPages",
          "b.pagesRead",
          "b.globalRating",
          "b.localRating",
          "b.userRating",
          "b.status",
          "b.inWishList",
          "b.language",
      })
      .from(kTableName, "b");

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

  query.select({"bg.book_isbn", "g.name"})
      .from("book_genres", "bg")
      .leftJoin("genres", "g")
      .on("g.id = bg.genre_id")
      .orderBy("g.name");

  auto rows = _db->select(query, &error);
  if (!error.isEmpty())
    qCWarning(lcBookTable) << "Failed to load genres:" << error;

  QHash<qint64, QStringList> result;
  for (const auto &row : std::as_const(rows)) {
    const QString name = row.value("name").toString();
    if (name.isEmpty())
      continue;

    const qint64 isbn = row.value("book_isbn").toLongLong();
    auto it = result.find(isbn);
    if (it == result.end())
      it = result.insert(isbn, {});
    it->append(name);
  }
  return result;
}

bool BookTable::setGenres(qint64 bookIsbn, const QStringList &genres) {
  core::SqlQueryBuilder clearQuery;
  QString error;

  // Replace rather than merge: the caller owns the whole list.
  clearQuery.deleteFrom("book_genres").where("book_isbn = ?").values({bookIsbn});
  if (_db->execute(clearQuery, &error) < 0) {
    qCWarning(lcBookTable) << "Failed to clear genres for book isbn:" << bookIsbn << "error:" << error;
    return false;
  }

  for (const QString &rawName : genres) {
    const QString name = rawName.trimmed();
    if (name.isEmpty())
      continue;

    core::SqlQueryBuilder addGenre;
    addGenre.insertOrIgnoreInto("genres", {"name"}).values({name});
    _db->insert(addGenre, &error); // already-present names return 0, which is fine

    core::SqlQueryBuilder findGenre;
    findGenre.select({"id"}).from("genres").where("name = ?").values({name});
    const auto rows = _db->select(findGenre, &error);
    if (rows.isEmpty()) {
      qCWarning(lcBookTable) << "Genre lookup failed after insert:" << name << "error:" << error;
      continue;
    }

    core::SqlQueryBuilder link;
    link.insertOrIgnoreInto("book_genres", {"book_isbn", "genre_id"})
        .values({bookIsbn, rows.first().value("id").toLongLong()});
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
      .insertInto(kTableName, {"isbn", "name", "author", "year", "publisher", "description", "coverUrl", "isHardcover",
                               "type", "totalPages", "pagesRead", "globalRating", "localRating", "userRating", "status",
                               "inWishList", "language"})
      .values({book.isbn, book.name, book.authorName, book.year, book.publisherName, book.description, book.coverUrl,
               book.isHardcover, book.typeName, book.totalPages, book.pagesRead, book.globalRating, book.localRating,
               book.userRating, book.status, book.inWishList, book.language});

  const qint64 inserted = _db->insert(query, &error);
  if (inserted <= 0) {
    qCWarning(lcBookTable) << "Failed to add book:" << book.name << "error:" << error;
    return 0;
  }

  qCInfo(lcBookTable) << "Added book isbn:" << book.isbn << "name:" << book.name;

  if (!book.genres.isEmpty())
    setGenres(book.isbn, book.genres);

  return book.isbn;
}

bool BookTable::updateBook(const BookDTO &book) {
  core::SqlQueryBuilder query;
  QString error;

  query.update(kTableName)
      .set({"name", "author", "year", "publisher", "description", "coverUrl", "isHardcover", "type", "totalPages",
            "pagesRead", "globalRating", "localRating", "userRating", "status", "inWishList", "language"})
      .where("isbn = ?")
      .values({book.name, book.authorName, book.year, book.publisherName, book.description, book.coverUrl,
               book.isHardcover, book.typeName, book.totalPages, book.pagesRead, book.globalRating, book.localRating,
               book.userRating, book.status, book.inWishList, book.language, book.isbn});

  const int affected = _db->execute(query, &error);

  if (affected > 0) {
    qCInfo(lcBookTable) << "Updated book isbn:" << book.isbn << "name:" << book.name;
    if (!book.genres.isEmpty())
      setGenres(book.isbn, book.genres);
    return true;
  }

  if (affected == 0)
    qCWarning(lcBookTable) << "No book found with isbn:" << book.isbn;
  else
    qCWarning(lcBookTable) << "Failed to update book isbn:" << book.isbn << "error:" << error;
  return false;
}

bool BookTable::deleteBook(qint64 isbn) {
  core::SqlQueryBuilder query;
  QString error;

  query.deleteFrom(kTableName).where("isbn = ?").values({isbn});

  const int affected = _db->execute(query, &error);

  if (affected > 0) {
    qCInfo(lcBookTable) << "Deleted book isbn:" << isbn;
    return true;
  }

  if (affected == 0)
    qCWarning(lcBookTable) << "No book found with isbn:" << isbn;
  else
    qCWarning(lcBookTable) << "Failed to delete book isbn:" << isbn << "error:" << error;
  return false;
}

QStringList BookTable::getGenres(qint64 bookIsbn) const {
  core::SqlQueryBuilder query;
  QString error;

  query.select({"g.name"})
      .from("book_genres", "bg")
      .leftJoin("genres", "g")
      .on("g.id = bg.genre_id")
      .where("bg.book_isbn = ?")
      .orderBy("g.name")
      .values({bookIsbn});

  auto rows = _db->select(query, &error);
  if (!error.isEmpty())
    qCWarning(lcBookTable) << "Failed to load genres for book isbn:" << bookIsbn << "error:" << error;

  QStringList result;
  result.reserve(rows.size());
  for (const auto &row : std::as_const(rows)) {
    result.append(row.value("name").toString());
  }
  return result;
}

QList<CharacterDTO> BookTable::getCharacters(qint64 bookIsbn) const {
  core::SqlQueryBuilder query;
  QString error;

  query.select({"id", "name", "role"}).from("book_characters").where("book_isbn = ?").orderBy("id").values({bookIsbn});

  auto rows = _db->select(query, &error);
  if (!error.isEmpty())
    qCWarning(lcBookTable) << "Failed to load characters for book isbn:" << bookIsbn << "error:" << error;

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

  query.update(kTableName).set({"pagesRead"}).where("isbn = ?").values({pagesRead, bookIsbn});
  const int affected = _db->execute(query, &error);

  if (affected > 0) {
    qCInfo(lcBookTable) << "Updated pagesRead — book isbn:" << bookIsbn << "value:" << pagesRead;
    return true;
  }
  if (affected == 0)
    qCWarning(lcBookTable) << "updatePagesRead: no book found with isbn:" << bookIsbn;
  else
    qCWarning(lcBookTable) << "updatePagesRead failed for book isbn:" << bookIsbn << "error:" << error;
  return false;
}

qint64 BookTable::insertReadingSession(qint64 bookIsbn, int pagesFrom, int pagesTo, int durationSeconds) {
  core::SqlQueryBuilder query;
  QString error;

  const QDateTime endedAt = QDateTime::currentDateTimeUtc();
  const QDateTime startedAt = endedAt.addSecs(-durationSeconds);

  query.insertInto("reading_sessions", {"book_isbn", "started_at", "ended_at", "pages_from", "pages_to"})
      .values({bookIsbn, startedAt.toString(Qt::ISODate), endedAt.toString(Qt::ISODate), pagesFrom, pagesTo});

  const qint64 id = _db->insert(query, &error);
  if (id > 0)
    qCInfo(lcBookTable) << "Inserted session — book isbn:" << bookIsbn << "duration(s):" << durationSeconds;
  else
    qCWarning(lcBookTable) << "insertReadingSession failed for book isbn:" << bookIsbn << "error:" << error;
  return id;
}

} // namespace readary::services
