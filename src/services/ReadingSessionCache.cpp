#include "ReadingSessionCache.hpp"
#include "ReadingPhase.hpp"

#include <QDateTime>
#include <QLoggingCategory>
#include <QSettings>

namespace {
Q_LOGGING_CATEGORY(lcReadingCache, "bl.services.readingCache")
} // namespace

namespace bl::services {

QString ReadingSessionCache::groupFor(qint64 bookId) { return QStringLiteral("readingSession/%1").arg(bookId); }

void ReadingSessionCache::save(qint64 bookId, int seconds, int phase) {
  if (bookId <= 0)
    return;

  QSettings settings;
  settings.beginGroup(groupFor(bookId));
  settings.setValue("seconds", seconds);
  settings.setValue("phase", phase);
  settings.setValue("lastSyncAt", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
  settings.endGroup();
  settings.sync();

  qCInfo(lcReadingCache) << "Saved bookId:" << bookId << "seconds:" << seconds << "phase:" << phase;
}

QVariantMap ReadingSessionCache::takeState(qint64 bookId) {
  if (bookId <= 0)
    return {};

  QSettings settings;
  settings.beginGroup(groupFor(bookId));
  if (!settings.contains("seconds")) {
    settings.endGroup();
    return {};
  }

  const int savedSeconds = settings.value("seconds").toInt();
  const int phase = settings.value("phase").toInt();
  const QDateTime lastSync = QDateTime::fromString(settings.value("lastSyncAt").toString(), Qt::ISODate);

  int adjustedSeconds = savedSeconds;
  if (phase == ReadingPhase::Running && lastSync.isValid()) {
    const qint64 elapsed = lastSync.secsTo(QDateTime::currentDateTimeUtc());
    if (elapsed > 0)
      adjustedSeconds += static_cast<int>(elapsed);
  }

  settings.remove("");
  settings.endGroup();
  settings.sync();

  qCInfo(lcReadingCache) << "Restored bookId:" << bookId << "seconds:" << adjustedSeconds << "phase:" << phase;

  return QVariantMap{
      {"seconds", adjustedSeconds},
      {"phase", phase},
  };
}

void ReadingSessionCache::clear(qint64 bookId) {
  if (bookId <= 0)
    return;

  QSettings settings;
  settings.beginGroup(groupFor(bookId));
  settings.remove("");
  settings.endGroup();
  settings.sync();
}

} // namespace bl::services
