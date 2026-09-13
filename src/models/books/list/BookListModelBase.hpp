#ifndef READARY_MODELS_BOOKS_BOOKLISTMODELBASE_HPP
#define READARY_MODELS_BOOKS_BOOKLISTMODELBASE_HPP

#include "services/dto/BookDTO.hpp"

#include <QAbstractListModel>

namespace readary::models {

class BookListModelBase : public QAbstractListModel {
  Q_OBJECT

public:
  enum Roles {
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
    LanguageRole,
    IsCustomRole,
    PdfPathRole,
    PdfSourceRole,
    GenresRole,
  };
  Q_ENUM(Roles)

  explicit BookListModelBase(QObject *parent);

  virtual void refresh() = 0;

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  void setBooks(const QList<services::BookDTO> &books);
  void appendBooks(const QList<services::BookDTO> &books);
  bool contains(qint64 isbn) const;
  services::BookDTO getBook(qint64 isbn) const;
  const QList<services::BookDTO> &books() const;

protected:
  QList<services::BookDTO> _books;
};

} // namespace readary::models

#endif // READARY_MODELS_BOOKS_BOOKLISTMODELBASE_HPP
