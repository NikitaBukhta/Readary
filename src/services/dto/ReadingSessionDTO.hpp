#ifndef READARY_SERVICES_READINGSESSIONDTO_HPP
#define READARY_SERVICES_READINGSESSIONDTO_HPP

#include <QDateTime>
#include <QVariantMap>
#include <QtTypes>

namespace readary::services {

// Pages read and duration are derived on read; the journal stores no totals.
struct ReadingSessionDTO {
  qint64 id = 0;
  qint64 bookIsbn = 0;
  QDateTime startedAt;
  QDateTime endedAt;
  int pagesFrom = 0;
  int pagesTo = 0;

  int pagesRead() const;
  int durationSeconds() const;

  static ReadingSessionDTO fromMap(const QVariantMap &data);
};

} // namespace readary::services

#endif // READARY_SERVICES_READINGSESSIONDTO_HPP
