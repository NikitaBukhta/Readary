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
      })
      .from(kTableName, "b");

  auto data = _db->select(query, &error);

  QList<BookDTO> result;
  result.reserve(data.size());
  const int rowIndex = 0;
  for (const auto &row : std::as_const(data)) {
    qDebug(lcBookTable) << rowIndex << ") " << row;
    result.emplaceBack(BookDTO::fromMap(row));
  }

  qCInfo(lcBookTable) << "Loaded" << result.size() << "books";
  return result;
}

qint64 BookTable::addBook(const BookDTO &book) {
  core::SqlQueryBuilder query;
  QString error;

  // isbn is the primary key and is supplied by the caller (not autoincremented).
  query
      .insertInto(kTableName,
                  {"isbn", "name", "author", "year", "publisher", "description", "coverUrl", "isHardcover", "type",
                   "totalPages", "pagesRead", "globalRating", "localRating", "userRating", "status", "inWishList"})
      .values({book.isbn, book.name, book.authorName, book.year, book.publisherName, book.description,
               book.coverUrl, book.isHardcover, book.typeName, book.totalPages, book.pagesRead,
               book.globalRating, book.localRating, book.userRating, book.status, book.inWishList});

  const qint64 inserted = _db->insert(query, &error);
  if (inserted > 0)
    qCInfo(lcBookTable) << "Added book isbn:" << book.isbn << "name:" << book.name;
  else
    qCWarning(lcBookTable) << "Failed to add book:" << book.name << "error:" << error;

  return inserted > 0 ? book.isbn : 0;
}

bool BookTable::updateBook(const BookDTO &book) {
  core::SqlQueryBuilder query;
  QString error;

  query.update(kTableName)
      .set({"name", "author", "year", "publisher", "description", "coverUrl", "isHardcover", "type", "totalPages",
            "pagesRead", "globalRating", "localRating", "userRating", "status", "inWishList"})
      .where("isbn = ?")
      .values({book.name, book.authorName, book.year, book.publisherName, book.description, book.coverUrl,
               book.isHardcover, book.typeName, book.totalPages, book.pagesRead, book.globalRating,
               book.localRating, book.userRating, book.status, book.inWishList, book.isbn});

  const int affected = _db->execute(query, &error);

  if (affected > 0) {
    qCInfo(lcBookTable) << "Updated book isbn:" << book.isbn << "name:" << book.name;
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

  query.select({"id", "name", "role"})
      .from("book_characters")
      .where("book_isbn = ?")
      .orderBy("id")
      .values({bookIsbn});

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
