#include "BookDTO.hpp"

namespace bl::services {

BookDTO BookDTO::fromMap(const QVariantMap &data) {
  BookDTO dto;
  dto.id = data.value("id").toLongLong();
  dto.name = data.value("name").toString();
  dto.authorId = data.value("author_id").toLongLong();
  dto.authorName = data.value("author").toString();
  dto.year = data.value("year").toInt();
  dto.publisherId = data.value("publisher_id").toLongLong();
  dto.publisherName = data.value("publisher").toString();
  dto.description = data.value("description").toString();
  dto.coverUrl = data.value("coverUrl").toString();
  dto.isHardcover = data.value("isHardcover").toBool();
  dto.typeId = data.value("type_id").toLongLong();
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

} // namespace bl::services
