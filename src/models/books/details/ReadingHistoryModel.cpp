#include "models/books/details/ReadingHistoryModel.hpp"

#include <QLoggingCategory>
#include <algorithm>

namespace {
Q_LOGGING_CATEGORY(lcHistoryModel, "readary.models.readingHistory")
}

namespace readary::models {

ReadingHistoryModel::ReadingHistoryModel(std::shared_ptr<services::BookTable> bookTable, QObject *parent)
    : QAbstractListModel{parent}, _bookTable{std::move(bookTable)} {}

int ReadingHistoryModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return _visibleCount;
}

QVariant ReadingHistoryModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= _visibleCount) {
    return {};
  }
  const services::ReadingSessionDTO &row = _allItems.at(index.row());
  switch (role) {
  case IdRole:
    return row.id;
  case StartedAtRole:
    return row.startedAt;
  case EndedAtRole:
    return row.endedAt;
  case PagesFromRole:
    return row.pagesFrom;
  case PagesToRole:
    return row.pagesTo;
  case PagesReadRole:
    return row.pagesRead();
  case DurationSecondsRole:
    return row.durationSeconds();
  default:
    return {};
  }
}

QHash<int, QByteArray> ReadingHistoryModel::roleNames() const {
  return {
      {IdRole, "id"},
      {StartedAtRole, "startedAt"},
      {EndedAtRole, "endedAt"},
      {PagesFromRole, "pagesFrom"},
      {PagesToRole, "pagesTo"},
      {PagesReadRole, "pagesRead"},
      {DurationSecondsRole, "durationSeconds"},
  };
}

bool ReadingHistoryModel::canLoadMore() const { return _visibleCount < _allItems.size(); }

bool ReadingHistoryModel::canHide() const { return _visibleCount > kPageSize; }

int ReadingHistoryModel::totalCount() const { return static_cast<int>(_allItems.size()); }

void ReadingHistoryModel::setBookIsbn(qint64 isbn) {
  const bool sameBook = _bookIsbn == isbn;
  _bookIsbn = isbn;
  reload(sameBook);
}

void ReadingHistoryModel::reload(bool keepExpansion) {
  const int previousVisible = _visibleCount;

  beginResetModel();
  _allItems = (_bookIsbn > 0) ? _bookTable->getReadingSessions(_bookIsbn) : QList<services::ReadingSessionDTO>{};
  const int wanted = keepExpansion ? std::max(kPageSize, previousVisible) : kPageSize;
  _visibleCount = std::min<int>(static_cast<int>(_allItems.size()), wanted);
  endResetModel();

  emit canLoadMoreChanged();
  emit canHideChanged();
  emit summaryChanged();

  qCInfo(lcHistoryModel) << "Loaded" << _allItems.size() << "sessions for book isbn:" << _bookIsbn
                         << "visible:" << _visibleCount;
}

void ReadingHistoryModel::loadMore() {
  if (!canLoadMore()) {
    return;
  }

  const int from = _visibleCount;
  const int newCount = std::min<int>(static_cast<int>(_allItems.size()), _visibleCount + kPageSize);
  const int to = newCount - 1;

  beginInsertRows({}, from, to);
  _visibleCount = newCount;
  endInsertRows();

  emit canLoadMoreChanged();
  emit canHideChanged();
}

void ReadingHistoryModel::hide() {
  if (!canHide()) {
    return;
  }

  const int newCount = std::min<int>(static_cast<int>(_allItems.size()), kPageSize);
  const int from = newCount;
  const int to = _visibleCount - 1;

  beginRemoveRows({}, from, to);
  _visibleCount = newCount;
  endRemoveRows();

  emit canLoadMoreChanged();
  emit canHideChanged();
}

} // namespace readary::models
