#ifndef READARY_SERVICES_BOOKSTATISTICSDTO_HPP
#define READARY_SERVICES_BOOKSTATISTICSDTO_HPP

#include "services/dto/JournalFiguresDTO.hpp"

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
struct BookStatisticsDTO : JournalFiguresDTO {
  // Seven buckets, Monday..Sunday of the reference week. Always seven, so the
  // chart has a column per weekday even for a book read nowhere near it.
  QList<int> weeklyPages;
  QList<ProgressPointDTO> progressPoints;

  // Lets the controller drop a recompute that changed nothing instead of
  // telling QML to rebuild the page.
  friend bool operator==(const BookStatisticsDTO &, const BookStatisticsDTO &) = default;
};

} // namespace readary::services

#endif // READARY_SERVICES_BOOKSTATISTICSDTO_HPP
