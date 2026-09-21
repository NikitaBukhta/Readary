#ifndef READARY_QMLTYPES_BOOKSTATISTICSOBJECT_HPP
#define READARY_QMLTYPES_BOOKSTATISTICSOBJECT_HPP

#include "services/dto/BookStatisticsDTO.hpp"

#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include <utility>

namespace readary::qmltypes {

struct BookStatisticsObject : public services::BookStatisticsDTO {
  Q_GADGET
  QML_VALUE_TYPE(bookStatisticsObject)

  Q_PROPERTY(int sessionCount MEMBER sessionCount)
  Q_PROPERTY(int timedSessionCount MEMBER timedSessionCount)
  Q_PROPERTY(int pagesRead MEMBER pagesRead)
  Q_PROPERTY(int totalSeconds MEMBER totalSeconds)
  Q_PROPERTY(double averagePagesPerHour MEMBER averagePagesPerHour)
  Q_PROPERTY(double minPagesPerHour MEMBER minPagesPerHour)
  Q_PROPERTY(double maxPagesPerHour MEMBER maxPagesPerHour)
  Q_PROPERTY(QList<int> weeklyPages MEMBER weeklyPages)
  // QList<ProgressPointDTO> is not a sequence QML knows how to walk, and
  // registering one would drag moc into the service DTO. The curve crosses as
  // an array of {session, page} objects instead.
  Q_PROPERTY(QVariantList progressPoints READ progressPointsAsVariantList)

public:
  BookStatisticsObject() = default;
  explicit BookStatisticsObject(const services::BookStatisticsDTO &base) : services::BookStatisticsDTO{base} {}
  explicit BookStatisticsObject(services::BookStatisticsDTO &&base) noexcept
      : services::BookStatisticsDTO{std::move(base)} {}

  QVariantList progressPointsAsVariantList() const {
    QVariantList points;
    points.reserve(progressPoints.size());
    for (const services::ProgressPointDTO &point : progressPoints) {
      points.append(QVariantMap{{QStringLiteral("session"), point.session}, {QStringLiteral("page"), point.page}});
    }
    return points;
  }
};

} // namespace readary::qmltypes

#endif // READARY_QMLTYPES_BOOKSTATISTICSOBJECT_HPP
