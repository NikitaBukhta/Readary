#ifndef READARY_QMLTYPES_READINGSTATISTICSOBJECT_HPP
#define READARY_QMLTYPES_READINGSTATISTICSOBJECT_HPP

#include "services/dto/ReadingStatisticsDTO.hpp"

#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include <utility>

namespace readary::qmltypes {

struct ReadingStatisticsObject : public services::ReadingStatisticsDTO {
  Q_GADGET
  QML_VALUE_TYPE(readingStatisticsObject)

  Q_PROPERTY(QString rangeStart READ rangeStart)
  Q_PROPERTY(QString rangeEnd READ rangeEnd)
  Q_PROPERTY(int granularity READ granularity)
  Q_PROPERTY(int booksFinished MEMBER booksFinished)
  Q_PROPERTY(int sessionCount MEMBER sessionCount)
  Q_PROPERTY(int timedSessionCount MEMBER timedSessionCount)
  Q_PROPERTY(int pagesRead MEMBER pagesRead)
  Q_PROPERTY(int totalSeconds MEMBER totalSeconds)
  Q_PROPERTY(double averagePagesPerHour MEMBER averagePagesPerHour)
  Q_PROPERTY(double minPagesPerHour MEMBER minPagesPerHour)
  Q_PROPERTY(double maxPagesPerHour MEMBER maxPagesPerHour)
  Q_PROPERTY(QVariantList buckets READ bucketsAsVariantList)
  Q_PROPERTY(QVariantList booksRead READ booksReadAsVariantList)

public:
  ReadingStatisticsObject() = default;
  explicit ReadingStatisticsObject(const services::ReadingStatisticsDTO &base) : services::ReadingStatisticsDTO{base} {}
  explicit ReadingStatisticsObject(services::ReadingStatisticsDTO &&base) noexcept
      : services::ReadingStatisticsDTO{std::move(base)} {}

  QString rangeStart() const { return range.from.toString(Qt::ISODate); }
  QString rangeEnd() const { return range.to.toString(Qt::ISODate); }
  int granularity() const { return static_cast<int>(range.granularity); }

  QVariantList bucketsAsVariantList() const {
    QVariantList rows;
    rows.reserve(buckets.size());
    for (const services::PeriodBucketDTO &bucket : buckets) {
      rows.append(QVariantMap{{QStringLiteral("year"), bucket.date.year()},
                              {QStringLiteral("month"), bucket.date.month()},
                              {QStringLiteral("day"), bucket.date.day()},
                              {QStringLiteral("hour"), bucket.hour},
                              {QStringLiteral("pages"), bucket.pages},
                              {QStringLiteral("books"), bucket.books}});
    }
    return rows;
  }

  QVariantList booksReadAsVariantList() const {
    QVariantList rows;
    rows.reserve(booksRead.size());
    for (const services::BookProgressDTO &book : booksRead) {
      rows.append(QVariantMap{{QStringLiteral("isbn"), book.isbn},
                              {QStringLiteral("name"), book.name},
                              {QStringLiteral("fromPage"), book.fromPage},
                              {QStringLiteral("toPage"), book.toPage},
                              {QStringLiteral("pagesInPeriod"), book.pagesInPeriod},
                              {QStringLiteral("totalPages"), book.totalPages}});
    }
    return rows;
  }
};

} // namespace readary::qmltypes

#endif // READARY_QMLTYPES_READINGSTATISTICSOBJECT_HPP
