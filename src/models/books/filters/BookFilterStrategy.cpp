#include "models/books/filters/BookFilterStrategy.hpp"

#include "models/books/list/BookListModel.hpp"
#include "models/books/proxy/BookSortFilterProxyModel.hpp"
#include "services/dto/BookStatus.hpp"

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
