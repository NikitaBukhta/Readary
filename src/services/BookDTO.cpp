#include "BookDTO.hpp"

using Qt::StringLiterals::operator""_s;

namespace readary::services {

BookDTO BookDTO::fromMap(const QVariantMap &data) {
  BookDTO dto;
  dto.isbn = data.value(u"isbn"_s).toLongLong();
  dto.name = data.value(u"name"_s).toString();
  dto.authorName = data.value(u"author"_s).toString();
  dto.year = data.value(u"year"_s).toInt();
  dto.publisherName = data.value(u"publisher"_s).toString();
  dto.description = data.value(u"description"_s).toString();
  dto.coverUrl = data.value(u"coverUrl"_s).toString();
  dto.isHardcover = data.value(u"isHardcover"_s).toBool();
  dto.typeName = data.value(u"type"_s).toString();
  dto.totalPages = data.value(u"totalPages"_s).toInt();
  dto.pagesRead = data.value(u"pagesRead"_s).toInt();
  dto.globalRating = data.value(u"globalRating"_s).toDouble();
  dto.localRating = data.value(u"localRating"_s).toDouble();
  dto.userRating = data.value(u"userRating"_s).toInt();
  dto.status = data.value(u"status"_s).toInt();
  dto.inWishList = data.value(u"inWishList"_s).toBool();
  dto.language = data.value(u"language"_s).toString();
  dto.isCustom = data.value(u"isCustom"_s).toBool();
  dto.pdfPath = data.value(u"pdfPath"_s).toString();
  dto.pdfSource = data.value(u"pdfSource"_s).toInt();
  dto.workKey = data.value(u"workKey"_s).toString();
  dto.genres = data.value(u"genres"_s).toStringList();
  return dto;
}

QVariantMap BookDTO::toMap() const {
  return {
      {u"isbn"_s, isbn},
      {u"name"_s, name},
      {u"author"_s, authorName},
      {u"year"_s, year},
      {u"publisher"_s, publisherName},
      {u"description"_s, description},
      {u"coverUrl"_s, coverUrl},
      {u"isHardcover"_s, isHardcover},
      {u"type"_s, typeName},
      {u"totalPages"_s, totalPages},
      {u"pagesRead"_s, pagesRead},
      {u"globalRating"_s, globalRating},
      {u"localRating"_s, localRating},
      {u"userRating"_s, userRating},
      {u"status"_s, status},
      {u"inWishList"_s, inWishList},
      {u"language"_s, language},
      {u"isCustom"_s, isCustom},
      {u"pdfPath"_s, pdfPath},
      {u"pdfSource"_s, pdfSource},
      {u"workKey"_s, workKey},
      {u"genres"_s, genres},
  };
}

} // namespace readary::services
