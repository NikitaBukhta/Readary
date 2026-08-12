#ifndef READARY_SERVICES_BOOKDTO_HPP
#define READARY_SERVICES_BOOKDTO_HPP

#include <QVariantMap>
#include <QtTypes>

namespace readary::services {

struct BookDTO {
  qint64 isbn = 0;
  QString name;
  QString authorName;
  int year = 0;
  QString publisherName;
  QString description;
  QString coverUrl;
  bool isHardcover = false;
  QString typeName;
  int totalPages = 0;
  int pagesRead = 0;
  double globalRating = 0.0;
  double localRating = 0.0;
  int userRating = 0;
  int status = 0;
  bool inWishList = false;
  QString language;
  QString workKey;
  QStringList genres;

  static BookDTO fromMap(const QVariantMap &data);
  QVariantMap toMap() const;
};

} // namespace readary::services

#endif // READARY_SERVICES_BOOKDTO_HPP
