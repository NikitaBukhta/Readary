#ifndef READARY_QMLTYPES_BOOKDTOOBJECT_HPP
#define READARY_QMLTYPES_BOOKDTOOBJECT_HPP

#include "services/BookDTO.hpp"

#include <QtQml/qqmlregistration.h>

#include <utility>

namespace readary::qmltypes {

struct BookDTOObject : public services::BookDTO {
  Q_GADGET
  QML_VALUE_TYPE(bookDtoObject)

  Q_PROPERTY(qint64 isbn MEMBER isbn)
  Q_PROPERTY(QString name MEMBER name)
  Q_PROPERTY(QString author MEMBER authorName)
  Q_PROPERTY(int year MEMBER year)
  Q_PROPERTY(QString publisher MEMBER publisherName)
  Q_PROPERTY(QString description MEMBER description)
  Q_PROPERTY(QString coverUrl MEMBER coverUrl)
  Q_PROPERTY(bool isHardcover MEMBER isHardcover)
  Q_PROPERTY(QString type MEMBER typeName)
  Q_PROPERTY(int totalPages MEMBER totalPages)
  Q_PROPERTY(int pagesRead MEMBER pagesRead)
  Q_PROPERTY(double globalRating MEMBER globalRating)
  Q_PROPERTY(double localRating MEMBER localRating)
  Q_PROPERTY(int userRating MEMBER userRating)
  Q_PROPERTY(int status MEMBER status)
  Q_PROPERTY(bool inWishList MEMBER inWishList)
  Q_PROPERTY(QString language MEMBER language)
  Q_PROPERTY(QStringList genres MEMBER genres)

public:
  BookDTOObject() = default;
  explicit BookDTOObject(const services::BookDTO &base) : services::BookDTO{base} {}
  explicit BookDTOObject(services::BookDTO &&base) noexcept : services::BookDTO{std::move(base)} {}
};

} // namespace readary::qmltypes

#endif // READARY_QMLTYPES_BOOKDTOOBJECT_HPP
