#include "services/caching/ReadingProgressCache.hpp"

#include <QLoggingCategory>
#include <QSettings>

using Qt::StringLiterals::operator""_s;

namespace {
Q_LOGGING_CATEGORY(lcProgressCache, "readary.services.progressCache")
} // namespace

namespace readary::services {

QString ReadingProgressCache::groupFor(qint64 bookIsbn) { return u"cachedProgress/%1"_s.arg(bookIsbn); }

void ReadingProgressCache::save(qint64 bookIsbn, int pagesRead) {
  if (bookIsbn <= 0) {
    return;
  }

  QSettings settings;
  settings.beginGroup(groupFor(bookIsbn));
  settings.setValue(u"pagesRead"_s, pagesRead);
  settings.endGroup();
  settings.sync();

  qCInfo(lcProgressCache) << "Saved bookIsbn:" << bookIsbn << "pagesRead:" << pagesRead;
}

bool ReadingProgressCache::has(qint64 bookIsbn) {
  if (bookIsbn <= 0) {
    return false;
  }

  QSettings settings;
  settings.beginGroup(groupFor(bookIsbn));
  const bool present = settings.contains(u"pagesRead"_s);
  settings.endGroup();
  return present;
}

int ReadingProgressCache::takePagesRead(qint64 bookIsbn) {
  if (bookIsbn <= 0) {
    return 0;
  }

  QSettings settings;
  settings.beginGroup(groupFor(bookIsbn));
  const int pagesRead = settings.value(u"pagesRead"_s, 0).toInt();
  settings.remove(QString{});
  settings.endGroup();
  settings.sync();

  qCInfo(lcProgressCache) << "Restored bookIsbn:" << bookIsbn << "pagesRead:" << pagesRead;
  return pagesRead;
}

void ReadingProgressCache::clear(qint64 bookIsbn) {
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
