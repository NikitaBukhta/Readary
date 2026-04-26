#ifndef BEELIBRARY_MODELS_BOOKPROXYMODEL_HPP
#define BEELIBRARY_MODELS_BOOKPROXYMODEL_HPP

#include "BookListModel.hpp"

#include <QJSEngine>
#include <QQmlEngine>
#include <QSortFilterProxyModel>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace bl::models {

class BookProxyModel : public QSortFilterProxyModel {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY
                 searchQueryChanged)
  Q_PROPERTY(
      int sortField READ sortRole WRITE setSortField NOTIFY sortFieldChanged)
  Q_PROPERTY(bool sortDescending READ sortDescending WRITE setSortDescending
                 NOTIFY sortDescendingChanged)

public:
  explicit BookProxyModel(QObject *parent);

  QString searchQuery() const;
  void setSearchQuery(const QString &query);

  void setSortField(int role);

  bool sortDescending() const;
  void setSortDescending(bool descending);

  static BookProxyModel *create(QQmlEngine *engine, QJSEngine *scriptEngine);
  static void setInstance(BookProxyModel *instance);

signals:
  void searchQueryChanged();
  void sortFieldChanged();
  void sortDescendingChanged();

protected:
  bool filterAcceptsRow(int sourceRow,
                        const QModelIndex &sourceParent) const override;
  bool lessThan(const QModelIndex &left,
                const QModelIndex &right) const override;

private:
  static BookProxyModel *s_instance;

  QString _searchQuery;
};

} // namespace bl::models

#endif // BEELIBRARY_MODELS_BOOKPROXYMODEL_HPP
