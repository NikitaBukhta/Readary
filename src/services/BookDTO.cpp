#include "BookDTO.hpp"

namespace readary::services {

BookDTO BookDTO::fromMap(const QVariantMap &data) {
  BookDTO dto;
  dto.isbn = data.value("isbn").toLongLong();
  dto.name = data.value("name").toString();
  dto.authorName = data.value("author").toString();
  dto.year = data.value("year").toInt();
  dto.publisherName = data.value("publisher").toString();
  dto.description = data.value("description").toString();
  dto.coverUrl = data.value("coverUrl").toString();
  dto.isHardcover = data.value("isHardcover").toBool();
  dto.typeName = data.value("type").toString();
  dto.totalPages = data.value("totalPages").toInt();
  dto.pagesRead = data.value("pagesRead").toInt();
  dto.globalRating = data.value("globalRating").toDouble();
  dto.localRating = data.value("localRating").toDouble();
  dto.userRating = data.value("userRating").toInt();
  dto.status = data.value("status").toInt();
  dto.inWishList = data.value("inWishList").toBool();
  return dto;
}

QVariantMap BookDTO::toMap() const {
  return {
      {"isbn", isbn},
      {"name", name},
      {"author", authorName},
      {"year", year},
      {"publisher", publisherName},
      {"description", description},
      {"coverUrl", coverUrl},
      {"isHardcover", isHardcover},
      {"type", typeName},
      {"totalPages", totalPages},
      {"pagesRead", pagesRead},
      {"globalRating", globalRating},
      {"localRating", localRating},
      {"userRating", userRating},
      {"status", status},
      {"inWishList", inWishList},
  };
}

} // namespace readary::services
