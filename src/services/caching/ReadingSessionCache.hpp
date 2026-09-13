#ifndef READARY_SERVICES_READINGSESSIONCACHE_HPP
#define READARY_SERVICES_READINGSESSIONCACHE_HPP

#include <QString>
#include <QVariantMap>

namespace readary::services {

// Persistent per-book reading-timer state, stored via QSettings so it
// survives both navigation away from the page AND app process exit.
//
// On `save`: writes { seconds, phase, lastSyncAt } under a per-bookIsbn group.
// On `takeState`: returns the saved snapshot, deletes the group, and — if
// the saved phase was Running — adds the wall-clock seconds elapsed
// between `lastSyncAt` and now to `seconds` (so a running timer keeps
// counting across app restarts).
//
// Not QML-visible by design — QML reaches this through `BookController`.
class ReadingSessionCache {
public:
  static void save(qint64 bookIsbn, int seconds, int phase);
  static QVariantMap takeState(qint64 bookIsbn);
  static void clear(qint64 bookIsbn);

private:
  static QString groupFor(qint64 bookIsbn);
};

} // namespace readary::services

#endif // READARY_SERVICES_READINGSESSIONCACHE_HPP
