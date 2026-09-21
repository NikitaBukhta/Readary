#ifndef READARY_SERVICES_BOOKSTATISTICSDTO_HPP
#define READARY_SERVICES_BOOKSTATISTICSDTO_HPP

#include <QList>

namespace readary::services {

// One point of the per-book progress curve: where the reader stood when a
// session ended.
struct ProgressPointDTO {
  int session = 0; // 1-based ordinal, oldest session first
  int page = 0;    // page reached when that session ended

  friend bool operator==(const ProgressPointDTO &, const ProgressPointDTO &) = default;
};

// Everything the statistics page shows about ONE book, derived from that
// book's reading_sessions journal. Nothing here is stored.
struct BookStatisticsDTO {
  static constexpr qsizetype kDaysInWeek = 7;

  int sessionCount = 0;
  // Sessions that carry a reading speed: the ones that were timed and logged
  // an end page. It exists so a displayed 0 p/h — a real, slow session — can
  // be told apart from "nothing was ever timed", which leaves the three
  // speeds at 0.0 as well.
  int timedSessionCount = 0;
  // Journal total, re-reads included — not the current reading position
  // (books.pagesRead is that; see db/init.sql).
  int pagesRead = 0;
  int totalSeconds = 0;
  double averagePagesPerHour = 0.0;
  double minPagesPerHour = 0.0;
  double maxPagesPerHour = 0.0;
  // Seven buckets, Monday..Sunday of the reference week. Always seven, so the
  // chart has a column per weekday even for a book read nowhere near it.
  QList<int> weeklyPages = QList<int>(kDaysInWeek, 0);
  QList<ProgressPointDTO> progressPoints;

  // Lets the controller drop a recompute that changed nothing instead of
  // telling QML to rebuild the page.
  friend bool operator==(const BookStatisticsDTO &, const BookStatisticsDTO &) = default;
};

} // namespace readary::services

#endif // READARY_SERVICES_BOOKSTATISTICSDTO_HPP
