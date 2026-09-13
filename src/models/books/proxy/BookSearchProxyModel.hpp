#ifndef READARY_MODELS_BOOKS_BOOKSEARCHPROXYMODEL_HPP
#define READARY_MODELS_BOOKS_BOOKSEARCHPROXYMODEL_HPP

#include "models/books/list/BookListModel.hpp"

#include <QSortFilterProxyModel>
#include <QString>
#include <QtQml/qqmlregistration.h>

#include <vector>

namespace readary::models {

class BookSearchProxyModel : public QSortFilterProxyModel {
  Q_OBJECT
  QML_ANONYMOUS

  Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)

public:
  explicit BookSearchProxyModel(QObject *parent = nullptr);

  QString searchQuery() const;
  void setSearchQuery(const QString &query);

signals:
  void searchQueryChanged();

protected:
  bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
  bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
  qint8 calculateMatchScore(const QModelIndex &index, const QString &query) const;
  qint8 cachedScore(int sourceRow) const;
  static qint8 scoreField(const QString &text, const QString &query, qint32 weight, qsizetype matchIdx);

  QString _searchQuery;

  // NOTE: in case of bug with searching during rowsInserted / rowsRemoved on the source model —
  // _searchCache is keyed by source row index. Qt does NOT re-call filterAcceptsRow for rows that
  // get shifted by an insert/remove, so cached scores end up associated with the wrong rows.
  // dataChanged and modelReset are handled correctly because Qt re-runs the filter for affected
  // rows, which lets filterAcceptsRow overwrite the cache. To fix the insert/remove case,
  // connect to source's rowsInserted/rowsRemoved and clear (or shift) the cache accordingly.
  mutable std::vector<qint8> _searchCache; // search match scores
};

} // namespace readary::models

#endif // READARY_MODELS_BOOKS_BOOKSEARCHPROXYMODEL_HPP
