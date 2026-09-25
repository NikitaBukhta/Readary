#ifndef READARY_SERVICES_READINGSTATISTICSDTO_HPP
#define READARY_SERVICES_READINGSTATISTICSDTO_HPP

#include "services/dto/JournalFiguresDTO.hpp"
#include "services/dto/StatisticsRange.hpp"

#include <QDate>
#include <QList>
#include <QString>
#include <QtTypes>

namespace readary::services {

struct PeriodBucketDTO {
  QDate date;
  int hour = 0;
  int pages = 0;
  int books = 0;

  friend bool operator==(const PeriodBucketDTO &, const PeriodBucketDTO &) = default;
};

struct BookProgressDTO {
  qint64 isbn = 0;
  QString name;
  int fromPage = 0;
  int toPage = 0;
  int pagesInPeriod = 0;
  int totalPages = 0;

  friend bool operator==(const BookProgressDTO &, const BookProgressDTO &) = default;
};

struct ReadingStatisticsDTO : JournalFiguresDTO {
  static constexpr qsizetype kBooksShown = 5;

  StatisticsRange range;

  int booksFinished = 0;
  QList<PeriodBucketDTO> buckets;
  QList<BookProgressDTO> booksRead;

  friend bool operator==(const ReadingStatisticsDTO &, const ReadingStatisticsDTO &) = default;
};

} // namespace readary::services

#endif // READARY_SERVICES_READINGSTATISTICSDTO_HPP
