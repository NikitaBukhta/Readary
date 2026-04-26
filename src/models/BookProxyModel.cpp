#include "BookProxyModel.hpp"

namespace bl::models {

BookProxyModel *BookProxyModel::s_instance = nullptr;

BookProxyModel::BookProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent) {
  setSortRole(BookListModel::NameRole);
  sort(0);
}

void BookProxyModel::setInstance(BookProxyModel *instance) {
  s_instance = instance;
}

BookProxyModel *BookProxyModel::create(QQmlEngine *, QJSEngine *) {
  Q_ASSERT_X(s_instance, "BookProxyModel::create",
             "setInstance() must be called before the QML engine loads");
  QQmlEngine::setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
  return s_instance;
}

QString BookProxyModel::searchQuery() const { return _searchQuery; }

void BookProxyModel::setSearchQuery(const QString &query) {
  if (_searchQuery == query)
    return;

  _searchQuery = query;
  emit searchQueryChanged();
  invalidateFilter();
}

void BookProxyModel::setSortField(int role) {
  if (sortRole() == role)
    return;

  setSortRole(role);
  invalidate();
  emit sortFieldChanged();
}

bool BookProxyModel::sortDescending() const {
  return sortOrder() == Qt::DescendingOrder;
}

void BookProxyModel::setSortDescending(bool descending) {
  auto order = descending ? Qt::DescendingOrder : Qt::AscendingOrder;
  if (sortOrder() == order)
    return;

  sort(0, order);
  emit sortDescendingChanged();
}

bool BookProxyModel::filterAcceptsRow(int sourceRow,
                                      const QModelIndex &sourceParent) const {
  if (_searchQuery.isEmpty())
    return true;

  QAbstractItemModel *model = sourceModel();
  QModelIndex idx = model->index(sourceRow, 0, sourceParent);

  return model->data(idx, BookListModel::NameRole)
             .toString()
             .contains(_searchQuery, Qt::CaseInsensitive) ||
         model->data(idx, BookListModel::AuthorRole)
             .toString()
             .contains(_searchQuery, Qt::CaseInsensitive) ||
         model->data(idx, BookListModel::DescriptionRole)
             .toString()
             .contains(_searchQuery, Qt::CaseInsensitive);
}

bool BookProxyModel::lessThan(const QModelIndex &left,
                              const QModelIndex &right) const {
  QVariant leftData = sourceModel()->data(left, sortRole());
  QVariant rightData = sourceModel()->data(right, sortRole());

  if (sortRole() == BookListModel::YearRole)
    return leftData.toInt() < rightData.toInt();

  return QString::localeAwareCompare(leftData.toString(),
                                     rightData.toString()) < 0;
}

} // namespace bl::models
