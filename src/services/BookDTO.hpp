#ifndef BEELIBRARY_SERVICES_BOOKDTO_HPP
#define BEELIBRARY_SERVICES_BOOKDTO_HPP

#include <QMap>
#include <QString>
#include <QVariant>
#include <QtTypes>

namespace bl::services {

// status: 0 = NONE, 1 = WantToRead, 2 = InProgress, 3 = Finished
struct BookDTO {
  qint64 id = 0;
  QString name;
  qint64 authorId = 0;
  QString authorName;
  int year = 0;
  qint64 publisherId = 0;
  QString publisherName;
  QString description;
  bool isHardcover = false;
  qint64 typeId = 0;
  QString typeName;
  int globalRating = 0;
  int localRating = 0;
  int userRating = 0;
  int status = 0;
  bool inWishList = false;

  QVariantMap toMap() const;
  static BookDTO fromMap(const QVariantMap &data);
};

} // namespace bl::services

#endif // BEELIBRARY_SERVICES_BOOKDTO_HPP
