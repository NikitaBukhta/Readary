#include "services/statistics/ReadingStatisticsCalculator.hpp"

#include "services/dto/BookStatus.hpp"
#include "services/statistics/BookStatisticsCalculator.hpp"

#include <QDateTime>
#include <QHash>

#include <algorithm>
#include <array>
#include <cstddef>
#include <utility>

namespace {

constexpr int g_monthsPerYear = 12;

int monthOrdinal(QDate date) { return (date.year() * g_monthsPerYear) + date.month() - 1; }

} // namespace

namespace readary::services {

ReadingStatisticsDTO ReadingStatisticsCalculator::compute(const QList<BookDTO> &books,
                                                          const QList<ReadingSessionDTO> &sessions, QDate today) {
  ReadingStatisticsDTO stats;

  // The journal arithmetic is the per-book one run over every book at once:
  // the week still buckets pages by day, and the speeds still come from the
  // timed sessions alone, so min <= average <= max holds here too.
  const BookStatisticsDTO journal = BookStatisticsCalculator::compute(sessions, today);
  stats.sessionCount = journal.sessionCount;
  stats.timedSessionCount = journal.timedSessionCount;
  stats.totalSeconds = journal.totalSeconds;
  stats.averagePagesPerHour = journal.averagePagesPerHour;
  stats.minPagesPerHour = journal.minPagesPerHour;
  stats.maxPagesPerHour = journal.maxPagesPerHour;
  stats.weeklyPages = journal.weeklyPages;

  QHash<qint64, QDateTime> lastRead;
  for (const ReadingSessionDTO &session : sessions) {
    const auto known = lastRead.constFind(session.bookIsbn);
    if (known == lastRead.cend() || session.endedAt > known.value()) {
      lastRead.insert(session.bookIsbn, session.endedAt);
    }
  }

  // Collected in a std::array for a bounds-checked at(), as in
  // BookStatisticsCalculator.
  std::array<int, static_cast<std::size_t>(ReadingStatisticsDTO::kMonthsShown)> monthBuckets{};
  const int firstMonth = today.isValid() ? monthOrdinal(today) - static_cast<int>(monthBuckets.size()) + 1 : 0;

  QList<BookDTO> inProgress;
  for (const BookDTO &book : books) {
    if (book.status == BookStatus::InProgress) {
      inProgress.append(book);
      continue;
    }
    if (book.status != BookStatus::Finished) {
      continue;
    }
    ++stats.booksFinished;

    // No finish date is stored: db/init.sql defines it as the end of the
    // book's last session. A book marked finished by hand, never timed, has
    // none — it counts towards booksFinished but lands in no month.
    const QDateTime finishedAt = lastRead.value(book.isbn);
    if (!today.isValid() || !finishedAt.isValid()) {
      continue;
    }
    const int offset = monthOrdinal(finishedAt.date()) - firstMonth;
    if (offset >= 0 && std::cmp_less(offset, monthBuckets.size())) {
      ++monthBuckets.at(static_cast<std::size_t>(offset));
    }
  }

  if (today.isValid()) {
    stats.monthlyBooks.reserve(ReadingStatisticsDTO::kMonthsShown);
    for (std::size_t i = 0; i < monthBuckets.size(); ++i) {
      const int ordinal = firstMonth + static_cast<int>(i);
      stats.monthlyBooks.append(
          {.year = ordinal / g_monthsPerYear, .month = (ordinal % g_monthsPerYear) + 1, .books = monthBuckets.at(i)});
    }
  }

  // Most recently read first — that is the book the reader is actually on.
  // Books never timed follow by name, and the key settles the rest so the
  // order, and with it the controller's change check, is deterministic.
  std::ranges::sort(inProgress, [&lastRead](const BookDTO &lhs, const BookDTO &rhs) {
    const QDateTime lhsRead = lastRead.value(lhs.isbn);
    const QDateTime rhsRead = lastRead.value(rhs.isbn);
    if (lhsRead != rhsRead) {
      return lhsRead.isValid() && (!rhsRead.isValid() || lhsRead > rhsRead);
    }
    const int byName = lhs.name.compare(rhs.name, Qt::CaseInsensitive);
    return byName != 0 ? byName < 0 : lhs.isbn < rhs.isbn;
  });

  const qsizetype shown = std::min(inProgress.size(), ReadingStatisticsDTO::kBooksInProgressShown);
  stats.booksInProgress.reserve(shown);
  for (qsizetype i = 0; i < shown; ++i) {
    const BookDTO &book = inProgress.at(i);
    stats.booksInProgress.append({.isbn = book.isbn,
                                  .name = book.name,
                                  .pagesRead = std::max(0, book.pagesRead),
                                  .totalPages = std::max(0, book.totalPages)});
  }

  return stats;
}

} // namespace readary::services
