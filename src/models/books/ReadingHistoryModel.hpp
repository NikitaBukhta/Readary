#ifndef READARY_MODELS_BOOKS_READINGHISTORYMODEL_HPP
#define READARY_MODELS_BOOKS_READINGHISTORYMODEL_HPP

#include "services/BookTable.hpp"
#include "services/ReadingSessionDTO.hpp"

#include <QAbstractListModel>
#include <QList>
#include <QtQml/qqmlregistration.h>
#include <memory>

namespace readary::models {

// Completed reading sessions of one book, newest first. The whole journal is
// pulled in one query and `_visibleCount` gates rowCount, so paging costs no SQL.
class ReadingHistoryModel : public QAbstractListModel {
  Q_OBJECT
  QML_ANONYMOUS

  Q_PROPERTY(bool canLoadMore READ canLoadMore NOTIFY canLoadMoreChanged)
  Q_PROPERTY(bool canHide READ canHide NOTIFY canHideChanged)
  Q_PROPERTY(int totalCount READ totalCount NOTIFY summaryChanged)

public:
  enum Roles {
    IdRole = Qt::UserRole + 1,
    StartedAtRole,
    EndedAtRole,
    PagesFromRole,
    PagesToRole,
    PagesReadRole,
    DurationSecondsRole,
  };
  Q_ENUM(Roles)

  explicit ReadingHistoryModel(std::shared_ptr<services::BookTable> bookTable, QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  bool canLoadMore() const;
  bool canHide() const;
  int totalCount() const;

  // Re-setting the ISBN already in place keeps however far the list was expanded.
  void setBookIsbn(qint64 isbn);

  Q_INVOKABLE void loadMore();
  Q_INVOKABLE void hide();

signals:
  void canLoadMoreChanged();
  void canHideChanged();
  void summaryChanged();

private:
  void reload(bool keepExpansion);

  static constexpr int kPageSize = 5;

  std::shared_ptr<services::BookTable> _bookTable;
  qint64 _bookIsbn = 0;
  QList<services::ReadingSessionDTO> _allItems;
  int _visibleCount = 0;
};

} // namespace readary::models

#endif // READARY_MODELS_BOOKS_READINGHISTORYMODEL_HPP
