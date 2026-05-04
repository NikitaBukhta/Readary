#include "BookTable.hpp"
#include "core/SqlQueryBuilder.hpp"

#include <QLoggingCategory>

namespace {
Q_LOGGING_CATEGORY(lcBookTable, "bl.services.books")
}

namespace bl::services {

const QString BookTable::kTableName = QStringLiteral("books");

BookTable::BookTable(std::shared_ptr<core::DatabaseManager> db) : _db{std::move(db)} {}

QList<BookDTO> BookTable::getAllBooks() {
  core::SqlQueryBuilder query;
  QString error;

  query
      .select({
          "b.id",
          "b.name",
          "b.author AS author_id",
          "a.name AS author",
          "b.year",
          "b.publisher AS publisher_id",
          "p.name AS publisher",
          "b.description",
          "b.coverUrl",
          "b.isHardcover",
          "b.type AS type_id",
          "t.name AS type",
          "b.totalPages",
          "b.pagesRead",
          "b.globalRating",
          "b.localRating",
          "b.userRating",
          "b.status",
          "b.inWishList",
      })
      .from(kTableName, "b")
      .leftJoin("authors", "a")
      .on("a.id = b.author")
      .leftJoin("publishers", "p")
      .on("p.id = b.publisher")
      .leftJoin("book_types", "t")
      .on("t.id = b.type");

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

namespace {

QVariant nullableId(qint64 id) { return id > 0 ? QVariant(id) : QVariant(); }

} // namespace

qint64 BookTable::addBook(const BookDTO &book) {
  core::SqlQueryBuilder query;
  QString error;

  query
      .insertInto(kTableName,
                  {"name", "author", "year", "publisher", "description", "coverUrl", "isHardcover", "type",
                   "totalPages", "pagesRead", "globalRating", "localRating", "userRating", "status", "inWishList"})
      .values({book.name, book.authorId, book.year, nullableId(book.publisherId), book.description, book.coverUrl,
               book.isHardcover, nullableId(book.typeId), book.totalPages, book.pagesRead, book.globalRating,
               book.localRating, book.userRating, book.status, book.inWishList});

  const qint64 id = _db->insert(query, &error);
  if (id > 0)
    qCInfo(lcBookTable) << "Added book id:" << id << "name:" << book.name;
  else
    qCWarning(lcBookTable) << "Failed to add book:" << book.name << "error:" << error;

  return id;
}

bool BookTable::updateBook(const BookDTO &book) {
  core::SqlQueryBuilder query;
  QString error;

  query.update(kTableName)
      .set({"name", "author", "year", "publisher", "description", "coverUrl", "isHardcover", "type", "totalPages",
            "pagesRead", "globalRating", "localRating", "userRating", "status", "inWishList"})
      .where("id = ?")
      .values({book.name, book.authorId, book.year, nullableId(book.publisherId), book.description, book.coverUrl,
               book.isHardcover, nullableId(book.typeId), book.totalPages, book.pagesRead, book.globalRating,
               book.localRating, book.userRating, book.status, book.inWishList, book.id});

  const int affected = _db->execute(query, &error);

  if (affected > 0) {
    qCInfo(lcBookTable) << "Updated book id:" << book.id << "name:" << book.name;
    return true;
  }

  if (affected == 0)
    qCWarning(lcBookTable) << "No book found with id:" << book.id;
  else
    qCWarning(lcBookTable) << "Failed to update book id:" << book.id << "error:" << error;
  return false;
}

bool BookTable::deleteBook(qint64 id) {
  core::SqlQueryBuilder query;
  QString error;

  query.deleteFrom(kTableName).where("id = ?").values({id});

  const int affected = _db->execute(query, &error);

  if (affected > 0) {
    qCInfo(lcBookTable) << "Deleted book id:" << id;
    return true;
  }

  if (affected == 0)
    qCWarning(lcBookTable) << "No book found with id:" << id;
  else
    qCWarning(lcBookTable) << "Failed to delete book id:" << id << "error:" << error;
  return false;
}

QStringList BookTable::getGenres(qint64 bookId) const {
  core::SqlQueryBuilder query;
  QString error;

  query.select({"g.name"})
      .from("book_genres", "bg")
      .leftJoin("genres", "g")
      .on("g.id = bg.genre_id")
      .where("bg.book_id = ?")
      .orderBy("g.name")
      .values({bookId});

  auto rows = _db->select(query, &error);
  if (!error.isEmpty())
    qCWarning(lcBookTable) << "Failed to load genres for book id:" << bookId << "error:" << error;

  QStringList result;
  result.reserve(rows.size());
  for (const auto &row : std::as_const(rows)) {
    result.append(row.value("name").toString());
  }
  return result;
}

QVariantList BookTable::getCharacters(qint64 bookId) const {
  core::SqlQueryBuilder query;
  QString error;

  query.select({"id", "name", "role"}).from("book_characters").where("book_id = ?").orderBy("id").values({bookId});

  auto rows = _db->select(query, &error);
  if (!error.isEmpty())
    qCWarning(lcBookTable) << "Failed to load characters for book id:" << bookId << "error:" << error;

  QVariantList result;
  result.reserve(rows.size());
  for (const auto &row : std::as_const(rows)) {
    result.append(row);
  }
  return result;
}

} // namespace bl::services
