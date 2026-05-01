#include "BookSearchProxyModel.hpp"

#include <array>

namespace {
struct SearchRoles {
  qint32 role;
  qint32 weight;
};

constexpr std::array<SearchRoles, 3> searchRoles = {{{bl::models::BookListModel::NameRole, 100},
                                                     {bl::models::BookListModel::AuthorRole, 60},
                                                     {bl::models::BookListModel::DescriptionRole, 20}}};

} // namespace

namespace bl::models {

BookSearchProxyModel::BookSearchProxyModel(QObject *parent) : QSortFilterProxyModel(parent) {
  setDynamicSortFilter(true);
  sort(0, Qt::AscendingOrder);
}

QString BookSearchProxyModel::searchQuery() const { return _searchQuery; }

void BookSearchProxyModel::setSearchQuery(const QString &query) {
  if (_searchQuery == query)
    return;

  _searchQuery = query;
  emit searchQueryChanged();
  invalidate();
}

bool BookSearchProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
  if (_searchQuery.isEmpty()) {
    return true;
  }

  QAbstractItemModel *model = sourceModel();
  if (!model)
    return false;

  QModelIndex idx = model->index(sourceRow, 0, sourceParent);
  if (!idx.isValid())
    return false;

  const qint8 matchScore = calculateMatchScore(idx, _searchQuery);
  const size_t localIndex = idx.row();

  if (localIndex >= _searchCache.size()) {
    _searchCache.resize(model->rowCount() + 1, 0); // +1 to avoid zero size;
  }

  _searchCache[localIndex] = matchScore;
  return matchScore > 0;
}

qint8 BookSearchProxyModel::calculateMatchScore(const QModelIndex &index, const QString &query) const {
  qint8 totalScore = 0;
  for (const auto &searchRole : searchRoles) {
    const QString fieldText = sourceModel()->data(index, searchRole.role).toString();
    const qsizetype matchIdx = fieldText.indexOf(query, 0, Qt::CaseInsensitive);
    if (matchIdx >= 0) {
      totalScore = scoreField(fieldText, query, searchRole.weight, matchIdx);
      break;
    }
  }

  return totalScore;
}

inline qint8 BookSearchProxyModel::scoreField(const QString &text, const QString &query, qint32 weight,
                                              qsizetype matchIdx) {
  // Coverage: query.size() / text.size() — fraction of the field covered by the query.
  // Position: 1 - matchIdx / text.size() — 1.0 at the start of the string, approaches zero toward the end.
  const double positionFactor = 1.0 - static_cast<double>(matchIdx) / text.size();
  return static_cast<qint8>(100.0 / text.size() * query.size() * weight / 100.0 * positionFactor);
}

bool BookSearchProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const {
  if (_searchQuery.size()) {
    const qint8 leftScore = cachedScore(left.row());
    const qint8 rightScore = cachedScore(right.row());

    if (leftScore != rightScore)
      return leftScore > rightScore;
  }

  return left.row() < right.row();
}

qint8 BookSearchProxyModel::cachedScore(int sourceRow) const {
  if (sourceRow < 0 || static_cast<size_t>(sourceRow) >= _searchCache.size())
    return 0;
  return _searchCache[sourceRow];
}

} // namespace bl::models
