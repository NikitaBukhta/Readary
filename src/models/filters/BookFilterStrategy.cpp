#include "BookFilterStrategy.hpp"
#include "models/BookListModel.hpp"
#include "models/BookSortFilterProxyModel.hpp"
#include "services/BookStatus.hpp"

namespace bl::models::filters {

void WantToReadFilterStrategy::apply(BookSortFilterProxyModel *proxy) const {
  proxy->clearFilter();
  proxy->addFilter(BookListModel::StatusRole, services::BookStatus::WantToRead);
}

void WantToBuyFilterStrategy::apply(BookSortFilterProxyModel *proxy) const {
  proxy->clearFilter();
  proxy->addFilter(BookListModel::InWishListRole, true);
}

void AlreadyReadFilterStrategy::apply(BookSortFilterProxyModel *proxy) const {
  proxy->clearFilter();
  proxy->addFilter(BookListModel::StatusRole, services::BookStatus::Finished);
}

void ReadInProgressFilterStrategy::apply(BookSortFilterProxyModel *proxy) const {
  proxy->clearFilter();
  proxy->addFilter(BookListModel::StatusRole, services::BookStatus::InProgress);
}

} // namespace bl::models::filters
