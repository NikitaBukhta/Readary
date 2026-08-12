#ifndef READARY_SERVICES_SEARCHCACHE_HPP
#define READARY_SERVICES_SEARCHCACHE_HPP

#include "BookDTO.hpp"

#include <QList>
#include <QString>
#include <optional>

namespace readary::services {

class SearchCache {
public:
  struct Entry {
    QList<BookDTO> books;
    int nextPage = 1;
    bool hasMore = true;
  };

  static std::optional<Entry> get(const QString &query, int maxAgeDays);
  static void put(const QString &query, const Entry &entry);

private:
  static QString groupFor(const QString &query);
};

} // namespace readary::services

#endif // READARY_SERVICES_SEARCHCACHE_HPP
