#ifndef READARY_QMLTYPES_LIBRARYSTATISTICSOBJECT_HPP
#define READARY_QMLTYPES_LIBRARYSTATISTICSOBJECT_HPP

#include "services/dto/LibraryStatisticsDTO.hpp"

#include <QObject>
#include <QtQml/qqmlregistration.h>

namespace readary::qmltypes {

struct LibraryStatisticsObject : public services::LibraryStatisticsDTO {
  Q_GADGET
  QML_VALUE_TYPE(libraryStatisticsObject)

  Q_PROPERTY(int booksTotal MEMBER booksTotal)
  Q_PROPERTY(int booksFinished MEMBER booksFinished)
  Q_PROPERTY(int booksInProgress MEMBER booksInProgress)
  Q_PROPERTY(int pagesRead MEMBER pagesRead)
  Q_PROPERTY(int totalSeconds MEMBER totalSeconds)
  Q_PROPERTY(int sessionCount MEMBER sessionCount)

public:
  LibraryStatisticsObject() = default;
  // Every member is an int, so there is nothing for a move overload to save.
  explicit LibraryStatisticsObject(const services::LibraryStatisticsDTO &base) : services::LibraryStatisticsDTO{base} {}
};

} // namespace readary::qmltypes

#endif // READARY_QMLTYPES_LIBRARYSTATISTICSOBJECT_HPP
