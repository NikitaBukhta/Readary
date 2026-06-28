#include "BookListModelBase.hpp"
#include <QLoggingCategory>
#include <algorithm>

namespace {
Q_LOGGING_CATEGORY(lcBookModelBase, "readary.models.booksBase")
}

namespace readary {
namespace models {

BookListModelBase::BookListModelBase(QObject *parent) : QAbstractListModel(parent) {
}

int BookListModelBase::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return static_cast<int>(_books.size());
}

QVariant BookListModelBase::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= _books.size())
    return {};

  const auto &book = _books.at(index.row());

  switch (role) {
  case IsbnRole:
    return book.isbn;
  case NameRole:
    return book.name;
  case AuthorRole:
    return book.authorName;
  case YearRole:
    return book.year;
  case PublisherRole:
    return book.publisherName;
  case DescriptionRole:
    return book.description;
  case CoverUrlRole:
    return book.coverUrl;
  case IsHardcoverRole:
    return book.isHardcover;
  case TypeRole:
    return book.typeName;
  case TotalPagesRole:
    return book.totalPages;
  case PagesReadRole:
    return book.pagesRead;
  case GlobalRatingRole:
    return book.globalRating;
  case LocalRatingRole:
    return book.localRating;
  case UserRatingRole:
    return book.userRating;
  case StatusRole:
    return book.status;
  case InWishListRole:
    return book.inWishList;
  default:
    return {};
  }
}

QHash<int, QByteArray> BookListModelBase::roleNames() const {
  return {
        {IsbnRole, "isbn"},
        {NameRole, "name"},
        {AuthorRole, "author"},
        {YearRole, "year"},
        {PublisherRole, "publisher"},
        {DescriptionRole, "description"},
        {CoverUrlRole, "coverUrl"},
        {IsHardcoverRole, "isHardcover"},
        {TypeRole, "type"},
        {TotalPagesRole, "totalPages"},
        {PagesReadRole, "pagesRead"},
        {GlobalRatingRole, "globalRating"},
        {LocalRatingRole, "localRating"},
        {UserRatingRole, "userRating"},
        {StatusRole, "status"},
        {InWishListRole, "inWishList"},
    };
}

services::BookDTO BookListModelBase::getBook(qint64 isbn) const {
  if (const auto it = std::find_if(_books.begin(), _books.end(),
                                   [isbn](const services::BookDTO &book) { return book.isbn == isbn; });
      it != _books.end()) {
    return *it;
  }
  qCWarning(lcBookModelBase) << "getBook — book not found, isbn:" << isbn;
  return {};
}

} // models
} // readary