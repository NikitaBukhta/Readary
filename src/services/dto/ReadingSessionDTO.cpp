#include "services/dto/ReadingSessionDTO.hpp"

#include <algorithm>

using Qt::StringLiterals::operator""_s;

namespace {

// SQLite hands TEXT timestamps back as strings unless the column was declared
// with a date type, so handle both. Journal stamps carry a zone (UTC); a
// zone-less one can only be hand-written, and is read as local time.
QDateTime toDateTime(const QVariant &value) {
  if (!value.isValid() || value.isNull()) {
    return {};
  }

  if (value.typeId() == QMetaType::QDateTime) {
    return value.toDateTime().toLocalTime();
  }

  const QDateTime parsed = QDateTime::fromString(value.toString(), Qt::ISODate);
  return parsed.isValid() ? parsed.toLocalTime() : QDateTime{};
}

} // namespace

namespace readary::services {

int ReadingSessionDTO::pagesRead() const { return std::max(0, pagesTo - pagesFrom); }

int ReadingSessionDTO::durationSeconds() const {
  if (!startedAt.isValid() || !endedAt.isValid()) {
    return 0;
  }
  return static_cast<int>(std::max<qint64>(0, startedAt.secsTo(endedAt)));
}

ReadingSessionDTO ReadingSessionDTO::fromMap(const QVariantMap &data) {
  ReadingSessionDTO dto;
  dto.id = data.value(u"id"_s).toLongLong();
  dto.bookIsbn = data.value(u"book_isbn"_s).toLongLong();
  dto.startedAt = toDateTime(data.value(u"started_at"_s));
  dto.endedAt = toDateTime(data.value(u"ended_at"_s));
  dto.pagesFrom = data.value(u"pages_from"_s).toInt();
  dto.pagesTo = data.value(u"pages_to"_s).toInt();
  return dto;
}

} // namespace readary::services
