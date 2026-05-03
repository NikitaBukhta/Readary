#include "BookSortFilterProxyModel.hpp"
#include "BookListModel.hpp"

namespace bl::models {

BookSortFilterProxyModel::BookSortFilterProxyModel(QObject *parent) : QSortFilterProxyModel(parent) {
  setSortRole(BookListModel::NameRole);
  setDynamicSortFilter(true);
  sort(0, Qt::AscendingOrder);

  connect(this, &QAbstractItemModel::rowsInserted, this, &BookSortFilterProxyModel::countChanged);
  connect(this, &QAbstractItemModel::rowsRemoved, this, &BookSortFilterProxyModel::countChanged);
  connect(this, &QAbstractItemModel::modelReset, this, &BookSortFilterProxyModel::countChanged);
  connect(this, &QAbstractItemModel::layoutChanged, this, &BookSortFilterProxyModel::countChanged);
}

void BookSortFilterProxyModel::addFilter(int role, const QVariant &value, Op op) {
  _filters.insert(role, Filter{value, op});
  invalidateFilter();
}

void BookSortFilterProxyModel::removeFilter(int role) {
  if (_filters.remove(role) > 0)
    invalidateFilter();
}

void BookSortFilterProxyModel::clearFilter() {
  if (_filters.isEmpty())
    return;
  _filters.clear();
  invalidateFilter();
}

void BookSortFilterProxyModel::setSortField(int role) {
  if (sortRole() == role)
    return;
  setSortRole(role);
  invalidate();
  emit sortFieldChanged();
}

bool BookSortFilterProxyModel::sortDescending() const { return sortOrder() == Qt::DescendingOrder; }

void BookSortFilterProxyModel::setSortDescending(bool descending) {
  auto order = descending ? Qt::DescendingOrder : Qt::AscendingOrder;
  if (sortOrder() == order)
    return;
  sort(0, order);
  emit sortDescendingChanged();
}

bool BookSortFilterProxyModel::matches(const QVariant &cell, const Filter &filter) {
  switch (filter.op) {
  case Op::Equal:
    return cell == filter.value;
  case Op::NotEqual:
    return cell != filter.value;
  case Op::Contains:
    return cell.toString().contains(filter.value.toString(), Qt::CaseInsensitive);
  case Op::Less:
  case Op::LessOrEqual:
  case Op::Greater:
  case Op::GreaterOrEqual: {
    bool okLhs = false;
    bool okRhs = false;
    const double lhs = cell.toDouble(&okLhs);
    const double rhs = filter.value.toDouble(&okRhs);
    if (!okLhs || !okRhs)
      return false;
    switch (filter.op) {
    case Op::Less:
      return lhs < rhs;
    case Op::LessOrEqual:
      return lhs <= rhs;
    case Op::Greater:
      return lhs > rhs;
    case Op::GreaterOrEqual:
      return lhs >= rhs;
    default:
      return false;
    }
  }
  }
  return false;
}

bool BookSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
  QAbstractItemModel *model = sourceModel();
  if (!model)
    return false;
  if (_filters.isEmpty())
    return true;

  QModelIndex idx = model->index(sourceRow, 0, sourceParent);

  // Same role -> OR; different roles -> AND.
  for (const int role : _filters.uniqueKeys()) {
    const QVariant cell = model->data(idx, role);
    auto range = _filters.equal_range(role);
    bool anyMatch = false;
    for (auto it = range.first; it != range.second; ++it) {
      if (matches(cell, it.value())) {
        anyMatch = true;
        break;
      }
    }
    if (!anyMatch)
      return false;
  }
  return true;
}

bool BookSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const {
  QVariant leftData = sourceModel()->data(left, sortRole());
  QVariant rightData = sourceModel()->data(right, sortRole());

  switch (sortRole()) {
  case BookListModel::YearRole:
  case BookListModel::TotalPagesRole:
  case BookListModel::PagesReadRole:
    return leftData.toInt() < rightData.toInt();
  case BookListModel::GlobalRatingRole:
  case BookListModel::LocalRatingRole:
  case BookListModel::UserRatingRole:
    return leftData.toDouble() < rightData.toDouble();
  default:
    return QString::localeAwareCompare(leftData.toString(), rightData.toString()) < 0;
  }
}

} // namespace bl::models
