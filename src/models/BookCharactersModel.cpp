#include "BookCharactersModel.hpp"

#include <QLoggingCategory>
#include <algorithm>

namespace {
Q_LOGGING_CATEGORY(lcCharactersModel, "bl.models.characters")
}

namespace bl::models {

BookCharactersModel::BookCharactersModel(std::shared_ptr<services::BookTable> bookTable, QObject *parent)
    : QAbstractListModel(parent), _bookTable{std::move(bookTable)} {}

int BookCharactersModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return _visibleCount;
}

QVariant BookCharactersModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= _visibleCount)
    return {};
  const services::CharacterDTO &row = _allItems.at(index.row());
  switch (role) {
  case IdRole:
    return row.id;
  case NameRole:
    return row.name;
  case RoleRole:
    return row.role;
  default:
    return {};
  }
}

QHash<int, QByteArray> BookCharactersModel::roleNames() const {
  return {
      {IdRole, "id"},
      {NameRole, "name"},
      {RoleRole, "role"},
  };
}

bool BookCharactersModel::canLoadMore() const { return _visibleCount < _allItems.size(); }

bool BookCharactersModel::canHide() const { return _visibleCount > kPageSize; }

void BookCharactersModel::setBookId(qint64 id) {
  beginResetModel();
  _bookId = id;
  _allItems = (id > 0) ? _bookTable->getCharacters(id) : QList<services::CharacterDTO>{};
  _visibleCount = std::min<int>(static_cast<int>(_allItems.size()), kPageSize);
  endResetModel();
  emit canLoadMoreChanged();
  emit canHideChanged();

  qCInfo(lcCharactersModel) << "setBookId" << id << "loaded" << _allItems.size() << "visible" << _visibleCount;
}

void BookCharactersModel::loadMore() {
  if (!canLoadMore())
    return;

  const int from = _visibleCount;
  const int newCount = std::min<int>(static_cast<int>(_allItems.size()), _visibleCount + kPageSize);
  const int to = newCount - 1;

  beginInsertRows(QModelIndex(), from, to);
  _visibleCount = newCount;
  endInsertRows();

  emit canLoadMoreChanged();
  emit canHideChanged();
}

void BookCharactersModel::hide() {
  if (!canHide())
    return;

  const int newCount = std::min<int>(static_cast<int>(_allItems.size()), kPageSize);
  const int from = newCount;
  const int to = _visibleCount - 1;

  beginRemoveRows(QModelIndex(), from, to);
  _visibleCount = newCount;
  endRemoveRows();

  emit canLoadMoreChanged();
  emit canHideChanged();
}

} // namespace bl::models
