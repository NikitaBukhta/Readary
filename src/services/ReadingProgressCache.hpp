#ifndef BEELIBRARY_SERVICES_READINGPROGRESSCACHE_HPP
#define BEELIBRARY_SERVICES_READINGPROGRESSCACHE_HPP

#include <QString>
#include <QtTypes>

namespace readary::services {

class ReadingProgressCache {
public:
  ReadingProgressCache() = default;

  static void save(qint64 bookIsbn, int pagesRead);
  static bool has(qint64 bookIsbn);
  static int takePagesRead(qint64 bookIsbn);
  static void clear(qint64 bookIsbn);

private:
  static QString groupFor(qint64 bookIsbn);
};

} // namespace readary::services

#endif // BEELIBRARY_SERVICES_READINGPROGRESSCACHE_HPP
