#ifndef BEELIBRARY_SERVICES_BOOKTABLE_HPP
#define BEELIBRARY_SERVICES_BOOKTABLE_HPP

#include "BookDTO.hpp"
#include "core/DatabaseManager.hpp"

#include <QString>
#include <memory>

namespace bl::services {

class BookTable {
public:
  explicit BookTable(std::shared_ptr<core::DatabaseManager> db);

  QList<BookDTO> getAllBooks();
  qint64 addBook(const BookDTO &book);
  bool updateBook(const BookDTO &book);
  bool deleteBook(qint64 id);

  QStringList getGenres(qint64 bookId) const;
  QVariantList getCharacters(qint64 bookId) const;

private:
  static const QString kTableName;
  std::shared_ptr<core::DatabaseManager> _db;
};

} // namespace bl::services

#endif // BEELIBRARY_SERVICES_BOOKTABLE_HPP
