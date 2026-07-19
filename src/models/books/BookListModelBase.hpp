#ifndef LIBRARY_BOOKLISTMODELBASE_H
#define LIBRARY_BOOKLISTMODELBASE_H

#include "services/BookDTO.hpp"

#include <QAbstractListModel>

namespace readary {
namespace models {

class BookListModelBase : public QAbstractListModel {
  Q_OBJECT

public:
  enum RolesEnum {
    IsbnRole = Qt::UserRole + 1,
    NameRole,
    AuthorRole,
    YearRole,
    PublisherRole,
    DescriptionRole,
    CoverUrlRole,
    IsHardcoverRole,
    TypeRole,
    TotalPagesRole,
    PagesReadRole,
    GlobalRatingRole,
    LocalRatingRole,
    UserRatingRole,
    StatusRole,
    InWishListRole,
  };
  Q_ENUM(RolesEnum)

  explicit BookListModelBase(QObject *parent);

  virtual void refresh() = 0;

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  void setBooks(const QList<services::BookDTO> &books);
  void appendBooks(const QList<services::BookDTO> &books);
  bool contains(qint64 isbn) const;
  services::BookDTO getBook(qint64 isbn) const;

protected:
  QList<services::BookDTO> _books;
};

} // namespace models
} // namespace readary

#endif // LIBRARY_BOOKLISTMODELBASE_H
