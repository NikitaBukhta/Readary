#include "ReadingSessionCache.hpp"

#include "ReadingPhase.hpp"

#include <QDateTime>
#include <QLoggingCategory>
#include <QSettings>

using Qt::StringLiterals::operator""_s;

namespace {
Q_LOGGING_CATEGORY(lcReadingCache, "readary.services.readingCache")
} // namespace

namespace readary::services {

QString ReadingSessionCache::groupFor(qint64 bookIsbn) { return u"readingSession/%1"_s.arg(bookIsbn); }

void ReadingSessionCache::save(qint64 bookIsbn, int seconds, int phase) {
  if (bookIsbn <= 0) {
    return;
  }

  QSettings settings;
  settings.beginGroup(groupFor(bookIsbn));
  settings.setValue(u"seconds"_s, seconds);
  settings.setValue(u"phase"_s, phase);
  settings.setValue(u"lastSyncAt"_s, QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
  settings.endGroup();
  settings.sync();

  qCInfo(lcReadingCache) << "Saved bookIsbn:" << bookIsbn << "seconds:" << seconds << "phase:" << phase;
}

QVariantMap ReadingSessionCache::takeState(qint64 bookIsbn) {
  if (bookIsbn <= 0) {
    return {};
  }

  QSettings settings;
  settings.beginGroup(groupFor(bookIsbn));
  if (!settings.contains(u"seconds"_s)) {
    settings.endGroup();
    return {};
  }

  const int savedSeconds = settings.value(u"seconds"_s).toInt();
  const int phase = settings.value(u"phase"_s).toInt();
  const QDateTime lastSync = QDateTime::fromString(settings.value(u"lastSyncAt"_s).toString(), Qt::ISODate);

  int adjustedSeconds = savedSeconds;
  if (phase == ReadingPhase::Running && lastSync.isValid()) {
    const qint64 elapsed = lastSync.secsTo(QDateTime::currentDateTimeUtc());
    if (elapsed > 0) {
      adjustedSeconds += static_cast<int>(elapsed);
    }
  }

  settings.remove(QString{});
  settings.endGroup();
  settings.sync();

  qCInfo(lcReadingCache) << "Restored bookIsbn:" << bookIsbn << "seconds:" << adjustedSeconds << "phase:" << phase;

  return QVariantMap{
      {u"seconds"_s, adjustedSeconds},
      {u"phase"_s, phase},
  };
}

void ReadingSessionCache::clear(qint64 bookIsbn) {
  if (bookIsbn <= 0) {
    return;
  }

  QSettings settings;
  settings.beginGroup(groupFor(bookIsbn));
  settings.remove(QString{});
  settings.endGroup();
  settings.sync();
}

} // namespace readary::services
