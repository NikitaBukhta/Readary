#ifndef READARY_SERVICES_READINGSTATISTICSDTO_HPP
#define READARY_SERVICES_READINGSTATISTICSDTO_HPP

#include <QList>
#include <QString>
#include <QtTypes>

namespace readary::services {

// Books finished in one calendar month.
struct MonthlyBooksDTO {
  int year = 0;
  int month = 0; // 1..12, QDate's numbering
  int books = 0;

  friend bool operator==(const MonthlyBooksDTO &, const MonthlyBooksDTO &) = default;
};

// Where the reader stands in one book that is still in progress.
struct BookProgressDTO {
  qint64 isbn = 0;
  QString name;
  int pagesRead = 0;
  int totalPages = 0; // 0 = unknown, as in BookDTO

  friend bool operator==(const BookProgressDTO &, const BookProgressDTO &) = default;
};

// Everything the reading-statistics page shows about the reader across the
// whole library: the per-book statistics page one level up. Nothing here is
// stored.
struct ReadingStatisticsDTO {
  static constexpr qsizetype kDaysInWeek = 7;
  static constexpr qsizetype kMonthsShown = 6;
  static constexpr qsizetype kBooksInProgressShown = 5;

  int booksFinished = 0;
  // Journal figures, with the same meaning as their BookStatisticsDTO
  // namesakes — only summed over every book instead of one.
  int sessionCount = 0;
  int timedSessionCount = 0;
  int totalSeconds = 0;
  double averagePagesPerHour = 0.0;
  double minPagesPerHour = 0.0;
  double maxPagesPerHour = 0.0;
  // Monday..Sunday of the reference week, every book together.
  QList<int> weeklyPages = QList<int>(kDaysInWeek, 0);
  // kMonthsShown entries, oldest first, ending with the reference month (none
  // without a valid reference) — so the chart keeps its axis even for a reader who finished nothing
  // lately.
  QList<MonthlyBooksDTO> monthlyBooks;
  // At most kBooksInProgressShown, most recently read first.
  QList<BookProgressDTO> booksInProgress;

  // Lets the controller drop a recompute that changed nothing instead of
  // telling QML to rebuild the page.
  friend bool operator==(const ReadingStatisticsDTO &, const ReadingStatisticsDTO &) = default;
};

} // namespace readary::services

#endif // READARY_SERVICES_READINGSTATISTICSDTO_HPP
