#include "BookFilterCriteria.hpp"

#include <algorithm>

namespace {

bool equalsAny(const QStringList &candidates, const QString &value) {
  return std::ranges::any_of(
      candidates, [&value](const QString &candidate) { return candidate.compare(value, Qt::CaseInsensitive) == 0; });
}

bool anyLanguageMatches(const QStringList &wanted, const QString &bookLanguages) {
  if (bookLanguages.isEmpty()) {
    return false;
  }
  QStringList actual = bookLanguages.split(u',', Qt::SkipEmptyParts);
  for (QString &code : actual) {
    code = code.trimmed();
  }
  return std::ranges::any_of(wanted, [&actual](const QString &want) { return equalsAny(actual, want.trimmed()); });
}

bool anyGenreMatches(const QStringList &wanted, const QStringList &actual) {
  return std::ranges::any_of(wanted, [&actual](const QString &want) { return equalsAny(actual, want); });
}

// A zero bound means "unbounded", which is why this can't just be min <= v <= max.
bool inRange(int value, int min, int max) {
  if (min > 0 && value < min) {
    return false;
  }
  if (max > 0 && value > max) {
    return false;
  }
  return true;
}

} // namespace

namespace readary::services {

bool BookFilterCriteria::isEmpty() const { return activeCount() == 0; }

int BookFilterCriteria::activeCount() const {
  int count = 0;
  count += !languages.isEmpty() ? 1 : 0;
  count += !genres.isEmpty() ? 1 : 0;
  count += !author.trimmed().isEmpty() ? 1 : 0;
  count += !publisher.trimmed().isEmpty() ? 1 : 0;
  // A range is one criterion, not one per bound.
  count += (minPages > 0 || maxPages > 0) ? 1 : 0;
  count += (minYear > 0 || maxYear > 0) ? 1 : 0;
  count += minRating > 0.0 ? 1 : 0;
  count += !statuses.isEmpty() ? 1 : 0;
  count += !types.isEmpty() ? 1 : 0;
  return count;
}

bool BookFilterCriteria::hasCatalogTerms() const {
  return !languages.isEmpty() || !genres.isEmpty() || !author.trimmed().isEmpty() || !publisher.trimmed().isEmpty();
}

BookFilterCriteria BookFilterCriteria::catalogSubset() const {
  BookFilterCriteria subset = *this;
  subset.statuses.clear();
  subset.types.clear();
  return subset;
}

bool BookFilterCriteria::matches(const BookDTO &book) const {
  if (!languages.isEmpty() && !anyLanguageMatches(languages, book.language)) {
    return false;
  }
  if (!genres.isEmpty() && !anyGenreMatches(genres, book.genres)) {
    return false;
  }
  if (const QString wanted = author.trimmed();
      !wanted.isEmpty() && !book.authorName.contains(wanted, Qt::CaseInsensitive)) {
    return false;
  }
  if (const QString wanted = publisher.trimmed();
      !wanted.isEmpty() && !book.publisherName.contains(wanted, Qt::CaseInsensitive)) {
    return false;
  }
  if (!inRange(book.totalPages, minPages, maxPages)) {
    return false;
  }
  if (!inRange(book.year, minYear, maxYear)) {
    return false;
  }
  if (minRating > 0.0 && book.globalRating < minRating) {
    return false;
  }
  if (!statuses.isEmpty() && !statuses.contains(book.status)) {
    return false;
  }
  if (!types.isEmpty() && !equalsAny(types, book.typeName)) {
    return false;
  }
  return true;
}

} // namespace readary::services
