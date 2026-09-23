#ifndef READARY_QMLTYPES_READINGSTATISTICSOBJECT_HPP
#define READARY_QMLTYPES_READINGSTATISTICSOBJECT_HPP

#include "services/dto/ReadingStatisticsDTO.hpp"

#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include <utility>

namespace readary::qmltypes {

struct ReadingStatisticsObject : public services::ReadingStatisticsDTO {
  Q_GADGET
  QML_VALUE_TYPE(readingStatisticsObject)

  Q_PROPERTY(int booksFinished MEMBER booksFinished)
  Q_PROPERTY(int sessionCount MEMBER sessionCount)
  Q_PROPERTY(int timedSessionCount MEMBER timedSessionCount)
  Q_PROPERTY(int totalSeconds MEMBER totalSeconds)
  Q_PROPERTY(double averagePagesPerHour MEMBER averagePagesPerHour)
  Q_PROPERTY(double minPagesPerHour MEMBER minPagesPerHour)
  Q_PROPERTY(double maxPagesPerHour MEMBER maxPagesPerHour)
  Q_PROPERTY(QList<int> weeklyPages MEMBER weeklyPages)
  // The two struct lists cross as arrays of plain objects, for the same reason
  // BookStatisticsObject::progressPoints does: registering them would drag moc
  // into the service DTO.
  Q_PROPERTY(QVariantList monthlyBooks READ monthlyBooksAsVariantList)
  Q_PROPERTY(QVariantList booksInProgress READ booksInProgressAsVariantList)

public:
  ReadingStatisticsObject() = default;
  explicit ReadingStatisticsObject(const services::ReadingStatisticsDTO &base) : services::ReadingStatisticsDTO{base} {}
  explicit ReadingStatisticsObject(services::ReadingStatisticsDTO &&base) noexcept
      : services::ReadingStatisticsDTO{std::move(base)} {}

  QVariantList monthlyBooksAsVariantList() const {
    QVariantList months;
    months.reserve(monthlyBooks.size());
    for (const services::MonthlyBooksDTO &month : monthlyBooks) {
      months.append(QVariantMap{{QStringLiteral("year"), month.year},
                                {QStringLiteral("month"), month.month},
                                {QStringLiteral("books"), month.books}});
    }
    return months;
  }

  QVariantList booksInProgressAsVariantList() const {
    QVariantList rows;
    rows.reserve(booksInProgress.size());
    for (const services::BookProgressDTO &book : booksInProgress) {
      rows.append(QVariantMap{{QStringLiteral("isbn"), book.isbn},
                              {QStringLiteral("name"), book.name},
                              {QStringLiteral("pagesRead"), book.pagesRead},
                              {QStringLiteral("totalPages"), book.totalPages}});
    }
    return rows;
  }
};

} // namespace readary::qmltypes

#endif // READARY_QMLTYPES_READINGSTATISTICSOBJECT_HPP
