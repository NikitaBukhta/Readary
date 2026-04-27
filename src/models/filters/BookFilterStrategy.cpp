#include "BookFilterStrategy.hpp"
#include "models/BookListModel.hpp"
#include "models/BookSortFilterProxyModel.hpp"

namespace bl::models::filters {

namespace {
constexpr int kStatusWantToRead = 1;
constexpr int kStatusInProgress = 2;
constexpr int kStatusFinished = 3;
} // namespace

void WantToReadFilterStrategy::apply(BookSortFilterProxyModel *proxy) const {
  proxy->clearFilter();
  proxy->addFilter(BookListModel::StatusRole, kStatusWantToRead);
}

void WantToBuyFilterStrategy::apply(BookSortFilterProxyModel *proxy) const {
  proxy->clearFilter();
  proxy->addFilter(BookListModel::InWishListRole, true);
}

void AlreadyReadFilterStrategy::apply(BookSortFilterProxyModel *proxy) const {
  proxy->clearFilter();
  proxy->addFilter(BookListModel::StatusRole, kStatusFinished);
}

void ReadInProgressFilterStrategy::apply(BookSortFilterProxyModel *proxy) const {
  proxy->clearFilter();
  proxy->addFilter(BookListModel::StatusRole, kStatusInProgress);
}

} // namespace bl::models::filters
