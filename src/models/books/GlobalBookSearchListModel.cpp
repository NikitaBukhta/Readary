#include "GlobalBookSearchListModel.hpp"

namespace readary::models {

GlobalBookSearchListModel::GlobalBookSearchListModel(QObject *parent) : BookListModelBase{parent} {}

void GlobalBookSearchListModel::refresh() { setBooks({}); }

void GlobalBookSearchListModel::onSearchListUpdated(const QList<services::BookDTO> &books) { setBooks(books); }

} // namespace readary::models
