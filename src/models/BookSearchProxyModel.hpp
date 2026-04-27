#ifndef BEELIBRARY_MODELS_BOOKSEARCHPROXYMODEL_HPP
#define BEELIBRARY_MODELS_BOOKSEARCHPROXYMODEL_HPP

#include "BookListModel.hpp"

#include <QSortFilterProxyModel>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace bl::models {

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

private:
  QString _searchQuery;
};

} // namespace bl::models

#endif // BEELIBRARY_MODELS_BOOKSEARCHPROXYMODEL_HPP
