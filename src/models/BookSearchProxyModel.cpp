#include "BookSearchProxyModel.hpp"

namespace bl::models {

BookSearchProxyModel::BookSearchProxyModel(QObject *parent) : QSortFilterProxyModel(parent) {
  setDynamicSortFilter(true);
}

QString BookSearchProxyModel::searchQuery() const { return _searchQuery; }

void BookSearchProxyModel::setSearchQuery(const QString &query) {
  if (_searchQuery == query)
    return;

  _searchQuery = query;
  emit searchQueryChanged();
  invalidateFilter();
}

bool BookSearchProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
  if (_searchQuery.isEmpty())
    return true;

  QAbstractItemModel *model = sourceModel();
  if (!model)
    return false;

  QModelIndex idx = model->index(sourceRow, 0, sourceParent);

  return model->data(idx, BookListModel::NameRole).toString().contains(_searchQuery, Qt::CaseInsensitive) ||
         model->data(idx, BookListModel::AuthorRole).toString().contains(_searchQuery, Qt::CaseInsensitive) ||
         model->data(idx, BookListModel::DescriptionRole).toString().contains(_searchQuery, Qt::CaseInsensitive);
}

} // namespace bl::models
