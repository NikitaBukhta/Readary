#ifndef BEELIBRARY_MODELS_BOOKCHARACTERSMODEL_HPP
#define BEELIBRARY_MODELS_BOOKCHARACTERSMODEL_HPP

#include "services/BookTable.hpp"
#include "services/CharacterDTO.hpp"

#include <QAbstractListModel>
#include <QList>
#include <QtQml/qqmlregistration.h>
#include <memory>

namespace readary::models {

class BookCharactersModel : public QAbstractListModel {
  Q_OBJECT
  QML_ANONYMOUS

  Q_PROPERTY(bool canLoadMore READ canLoadMore NOTIFY canLoadMoreChanged)
  Q_PROPERTY(bool canHide READ canHide NOTIFY canHideChanged)

public:
  enum Roles {
    IdRole = Qt::UserRole + 1,
    NameRole,
    RoleRole,
  };
  Q_ENUM(Roles)

  explicit BookCharactersModel(std::shared_ptr<services::BookTable> bookTable, QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  bool canLoadMore() const;
  bool canHide() const;

  void setBookIsbn(qint64 isbn);

  Q_INVOKABLE void loadMore();
  Q_INVOKABLE void hide();

signals:
  void canLoadMoreChanged();
  void canHideChanged();

private:
  static constexpr int kPageSize = 5;

  std::shared_ptr<services::BookTable> _bookTable;
  qint64 _bookIsbn = 0;
  QList<services::CharacterDTO> _allItems;
  int _visibleCount = 0;
};

} // namespace readary::models

#endif // BEELIBRARY_MODELS_BOOKCHARACTERSMODEL_HPP
