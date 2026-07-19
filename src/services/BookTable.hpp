#ifndef BEELIBRARY_SERVICES_BOOKTABLE_HPP
#define BEELIBRARY_SERVICES_BOOKTABLE_HPP

#include "BookDTO.hpp"
#include "CharacterDTO.hpp"
#include "core/DatabaseManager.hpp"

#include <QString>
#include <memory>

namespace readary::services {

class BookTable {
public:
  explicit BookTable(std::shared_ptr<core::DatabaseManager> db);

  QList<BookDTO> getAllBooks();
  qint64 addBook(const BookDTO &book);
  bool updateBook(const BookDTO &book);
  bool deleteBook(qint64 isbn);

  QStringList getGenres(qint64 bookIsbn) const;
  QList<CharacterDTO> getCharacters(qint64 bookIsbn) const;

  bool updatePagesRead(qint64 bookIsbn, int pagesRead);
  qint64 insertReadingSession(qint64 bookIsbn, int pagesFrom, int pagesTo, int durationSeconds);

private:
  static const QString kTableName;
  std::shared_ptr<core::DatabaseManager> _db;
};

} // namespace readary::services

#endif // BEELIBRARY_SERVICES_BOOKTABLE_HPP
