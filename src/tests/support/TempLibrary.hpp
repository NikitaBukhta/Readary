#ifndef READARY_TESTS_SUPPORT_TEMPLIBRARY_HPP
#define READARY_TESTS_SUPPORT_TEMPLIBRARY_HPP

#include "core/DatabaseManager.hpp"
#include "services/BookDTO.hpp"
#include "services/BookFileStore.hpp"
#include "services/BookStatus.hpp"
#include "services/BookTable.hpp"

#include <QString>
#include <QTemporaryDir>
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

    // Rebuilt per open() so files named after an isbn cannot survive into the
    // next test function.
    _filesDir = std::make_unique<QTemporaryDir>();
    if (!_filesDir->isValid()) {
      close();
      return false;
    }
    _files = std::make_shared<services::BookFileStore>(_filesDir->path());
    return true;
  }

  void close() {
    _files.reset();
    _filesDir.reset();
    _books.reset();
    _db.reset();
  }

  const std::shared_ptr<core::DatabaseManager> &db() const { return _db; }
  const std::shared_ptr<services::BookTable> &books() const { return _books; }
  const std::shared_ptr<services::BookFileStore> &files() const { return _files; }
  QString filesRoot() const { return _filesDir ? _filesDir->path() : QString{}; }

private:
  std::shared_ptr<core::DatabaseManager> _db;
  std::shared_ptr<services::BookTable> _books;
  std::unique_ptr<QTemporaryDir> _filesDir;
  std::shared_ptr<services::BookFileStore> _files;
};

// A book that satisfies every CHECK constraint in the schema, with a real page
// count so tests that read progress have something to work against.
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
