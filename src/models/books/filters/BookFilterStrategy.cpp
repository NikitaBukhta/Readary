#include "BookFilterStrategy.hpp"

#include "models/books/BookListModel.hpp"
#include "models/books/BookSortFilterProxyModel.hpp"
#include "services/BookStatus.hpp"

namespace readary::models::filters {

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

} // namespace readary::models::filters
