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
  // Returns the row's key. A book with a known ISBN is stored under it; one
  // without (`isbn <= 0`) is keyed by nextLocalKey() instead — no ISBN is ever
  // invented for it.
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

  // Key for a book with no ISBN. Counts up from 1, so it stays far below the
  // ISBN-13 range and can neither be mistaken for an ISBN nor collide with a
  // real one imported later.
  qint64 nextLocalKey() const;

  static constexpr QLatin1StringView kTableName{"books"};
  // Every ISBN-13 is 978- or 979-prefixed, so none is below this.
  static constexpr qint64 kIsbn13Floor = 9'780'000'000'000;

  std::shared_ptr<core::DatabaseManager> _db;
};

} // namespace readary::services

#endif // READARY_SERVICES_BOOKTABLE_HPP
