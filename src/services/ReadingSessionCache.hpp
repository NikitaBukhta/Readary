#ifndef BEELIBRARY_SERVICES_READINGSESSIONCACHE_HPP
#define BEELIBRARY_SERVICES_READINGSESSIONCACHE_HPP

#include <QString>
#include <QVariantMap>

namespace bl::services {

// Persistent per-book reading-timer state, stored via QSettings so it
// survives both navigation away from the page AND app process exit.
//
// On `save`: writes { seconds, phase, lastSyncAt } under a per-bookId group.
// On `takeState`: returns the saved snapshot, deletes the group, and — if
// the saved phase was Running — adds the wall-clock seconds elapsed
// between `lastSyncAt` and now to `seconds` (so a running timer keeps
// counting across app restarts).
//
// Not QML-visible by design — QML reaches this through `BookController`.
class ReadingSessionCache {
public:
  ReadingSessionCache() = default;

  static void save(qint64 bookId, int seconds, int phase);
  static QVariantMap takeState(qint64 bookId);
  static void clear(qint64 bookId);

private:
  static QString groupFor(qint64 bookId);
};

} // namespace bl::services

#endif // BEELIBRARY_SERVICES_READINGSESSIONCACHE_HPP
