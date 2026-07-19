#include "ReadingSessionCache.hpp"
#include "ReadingPhase.hpp"

#include <QDateTime>
#include <QLoggingCategory>
#include <QSettings>

namespace {
Q_LOGGING_CATEGORY(lcReadingCache, "readary.services.readingCache")
} // namespace

namespace readary::services {

QString ReadingSessionCache::groupFor(qint64 bookIsbn) { return QStringLiteral("readingSession/%1").arg(bookIsbn); }

void ReadingSessionCache::save(qint64 bookIsbn, int seconds, int phase) {
  if (bookIsbn <= 0)
    return;

  QSettings settings;
  settings.beginGroup(groupFor(bookIsbn));
  settings.setValue("seconds", seconds);
  settings.setValue("phase", phase);
  settings.setValue("lastSyncAt", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
  settings.endGroup();
  settings.sync();

  qCInfo(lcReadingCache) << "Saved bookIsbn:" << bookIsbn << "seconds:" << seconds << "phase:" << phase;
}

QVariantMap ReadingSessionCache::takeState(qint64 bookIsbn) {
  if (bookIsbn <= 0)
    return {};

  QSettings settings;
  settings.beginGroup(groupFor(bookIsbn));
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

  qCInfo(lcReadingCache) << "Restored bookIsbn:" << bookIsbn << "seconds:" << adjustedSeconds << "phase:" << phase;

  return QVariantMap{
      {"seconds", adjustedSeconds},
      {"phase", phase},
  };
}

void ReadingSessionCache::clear(qint64 bookIsbn) {
  if (bookIsbn <= 0)
    return;

  QSettings settings;
  settings.beginGroup(groupFor(bookIsbn));
  settings.remove("");
  settings.endGroup();
  settings.sync();
}

} // namespace readary::services
