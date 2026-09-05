#ifndef READARY_SERVICES_BOOKTABLE_HPP
#define READARY_SERVICES_BOOKTABLE_HPP

#include "BookDTO.hpp"
#include "CharacterDTO.hpp"
#include "ReadingSessionDTO.hpp"
#include "core/DatabaseManager.hpp"

#include <QHash>
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
  bool setGenres(qint64 bookIsbn, const QStringList &genres);

  QList<CharacterDTO> getCharacters(qint64 bookIsbn) const;
  QList<ReadingSessionDTO> getReadingSessions(qint64 bookIsbn) const;

  bool updatePagesRead(qint64 bookIsbn, int pagesRead);
  qint64 insertReadingSession(qint64 bookIsbn, int pagesFrom, int pagesTo, int durationSeconds);
  bool deleteReadingSession(qint64 sessionId);

private:
  QHash<qint64, QStringList> getGenresByBook() const;

  static constexpr QLatin1StringView kTableName{"books"};

  std::shared_ptr<core::DatabaseManager> _db;
};

} // namespace readary::services

#endif // READARY_SERVICES_BOOKTABLE_HPP
