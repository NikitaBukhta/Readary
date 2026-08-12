#include "BookCriteriaFilterProxyModel.hpp"
#include "BookListModelBase.hpp"

namespace readary::models {

BookCriteriaFilterProxyModel::BookCriteriaFilterProxyModel(QObject *parent) : QSortFilterProxyModel{parent} {
  setDynamicSortFilter(true);

  connect(this, &QAbstractItemModel::rowsInserted, this, &BookCriteriaFilterProxyModel::countChanged);
  connect(this, &QAbstractItemModel::rowsRemoved, this, &BookCriteriaFilterProxyModel::countChanged);
  connect(this, &QAbstractItemModel::modelReset, this, &BookCriteriaFilterProxyModel::countChanged);
  connect(this, &QAbstractItemModel::layoutChanged, this, &BookCriteriaFilterProxyModel::countChanged);
}

const services::BookFilterCriteria &BookCriteriaFilterProxyModel::criteria() const { return _criteria; }

void BookCriteriaFilterProxyModel::setCriteria(const services::BookFilterCriteria &criteria) {
  _criteria = criteria;
  invalidateFilter();
}

void BookCriteriaFilterProxyModel::clearCriteria() {
  if (_criteria.isEmpty()) {
    return;
  }
  _criteria = {};
  invalidateFilter();
}

bool BookCriteriaFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
  const QAbstractItemModel *model = sourceModel();
  if (model == nullptr) {
    return false;
  }
  if (_criteria.isEmpty()) {
    return true;
  }

  const QModelIndex idx = model->index(sourceRow, 0, sourceParent);

  services::BookDTO book;
  book.authorName = model->data(idx, BookListModelBase::AuthorRole).toString();
  book.publisherName = model->data(idx, BookListModelBase::PublisherRole).toString();
  book.language = model->data(idx, BookListModelBase::LanguageRole).toString();
  book.genres = model->data(idx, BookListModelBase::GenresRole).toStringList();
  book.typeName = model->data(idx, BookListModelBase::TypeRole).toString();
  book.year = model->data(idx, BookListModelBase::YearRole).toInt();
  book.totalPages = model->data(idx, BookListModelBase::TotalPagesRole).toInt();
  book.globalRating = model->data(idx, BookListModelBase::GlobalRatingRole).toDouble();
  book.status = model->data(idx, BookListModelBase::StatusRole).toInt();

  return _criteria.matches(book);
}

} // namespace readary::models
