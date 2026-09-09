#ifndef READARY_TESTS_SUPPORT_TEMPLIBRARY_HPP
#define READARY_TESTS_SUPPORT_TEMPLIBRARY_HPP

#include "core/DatabaseManager.hpp"
#include "services/BookDTO.hpp"
#include "services/BookStatus.hpp"
#include "services/BookTable.hpp"

#include <QString>
#include <memory>

namespace readary::tests {

// An in-memory library holding the real schema from :/db/init.sql and nothing
// else. DatabaseManager registers one fixed Qt SQL connection name, so only a
// single instance may be alive at a time — open() from init(), close() from
// cleanup(). The caller must have run Q_INIT_RESOURCE(db_scripts) first.
class TempLibrary {
public:
  bool open() {
    _db = std::make_shared<core::DatabaseManager>(QStringLiteral(":memory:"));
    if (!_db->open() || !_db->runScript(QStringLiteral(":/db/init.sql"))) {
      close();
      return false;
    }
    _books = std::make_shared<services::BookTable>(_db);
    return true;
  }

  void close() {
    _books.reset();
    _db.reset();
  }

  const std::shared_ptr<core::DatabaseManager> &db() const { return _db; }
  const std::shared_ptr<services::BookTable> &books() const { return _books; }

private:
  std::shared_ptr<core::DatabaseManager> _db;
  std::shared_ptr<services::BookTable> _books;
};

// A book that satisfies every CHECK constraint in the schema — notably
// totalPages, which the schema rejects at the DTO's own default of 0.
inline services::BookDTO makeBook(qint64 isbn, const QString &name, int status = services::BookStatus::None) {
  services::BookDTO book;
  book.isbn = isbn;
  book.name = name;
  book.authorName = QStringLiteral("Author");
  book.year = 2001;
  book.publisherName = QStringLiteral("Publisher");
  book.totalPages = 300;
  book.status = status;
  book.language = QStringLiteral("en");
  book.typeName = QStringLiteral("basic");
  return book;
}

} // namespace readary::tests

#endif // READARY_TESTS_SUPPORT_TEMPLIBRARY_HPP
