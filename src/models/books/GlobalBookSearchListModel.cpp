#include "GlobalBookSearchListModel.hpp"

namespace readary::models {

GlobalBookSearchListModel::GlobalBookSearchListModel(QObject *parent) : BookListModelBase{parent} {}

void GlobalBookSearchListModel::refresh() { setBooks({}); }

} // namespace readary::models
