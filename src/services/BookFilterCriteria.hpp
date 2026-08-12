#ifndef READARY_SERVICES_BOOKFILTERCRITERIA_HPP
#define READARY_SERVICES_BOOKFILTERCRITERIA_HPP

#include "BookDTO.hpp"

#include <QList>
#include <QString>
#include <QStringList>

namespace readary::services {

struct BookFilterCriteria {
  QStringList languages; // ISO 639-1; any match passes
  QStringList genres;
  QString author;
  QString publisher;

  int minPages = 0;
  int maxPages = 0;
  int minYear = 0;
  int maxYear = 0;

  double minRating = 0.0;

  QList<int> statuses;
  QStringList types;

  bool isEmpty() const;
  int activeCount() const;
  bool matches(const BookDTO &book) const;
  bool hasCatalogTerms() const;
  BookFilterCriteria catalogSubset() const;
};

} // namespace readary::services

#endif // READARY_SERVICES_BOOKFILTERCRITERIA_HPP
