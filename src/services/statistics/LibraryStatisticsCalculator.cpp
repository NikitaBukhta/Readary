#include "services/statistics/LibraryStatisticsCalculator.hpp"

#include "services/dto/BookStatus.hpp"

#include <algorithm>

namespace {

// A book the reader finished counts whole even when its position was never
// written — marking it finished by hand leaves pagesRead behind. The other way
// round, a position past the stated length (imported page count too low) is
// taken at face value rather than clipped. An unknown length reads back as 0,
// which the same comparison already handles.
int pagesBehind(const readary::services::BookDTO &book) {
  const int position = std::max(0, book.pagesRead);
  return book.status == readary::services::BookStatus::Finished ? std::max(position, book.totalPages) : position;
}

} // namespace

namespace readary::services {

LibraryStatisticsDTO LibraryStatisticsCalculator::compute(const QList<BookDTO> &books,
                                                          const QList<ReadingSessionDTO> &sessions) {
  LibraryStatisticsDTO stats;
  stats.booksTotal = static_cast<int>(books.size());

  for (const BookDTO &book : books) {
    if (book.status == BookStatus::Finished) {
      ++stats.booksFinished;
    } else if (book.status == BookStatus::InProgress) {
      ++stats.booksInProgress;
    }
    stats.pagesRead += pagesBehind(book);
  }

  stats.sessionCount = static_cast<int>(sessions.size());
  for (const ReadingSessionDTO &session : sessions) {
    stats.totalSeconds += session.durationSeconds();
  }

  return stats;
}

} // namespace readary::services
