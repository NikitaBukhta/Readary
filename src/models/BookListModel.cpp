#include "BookListModel.hpp"

#include <QLoggingCategory>

namespace {
Q_LOGGING_CATEGORY(lcBookModel, "bl.models.books")
}

namespace bl::models {

BookListModel::BookListModel(std::shared_ptr<services::BookTable> bookTable, QObject *parent)
    : QAbstractListModel(parent), _bookTable{std::move(bookTable)} {
  refresh();
  qCInfo(lcBookModel) << "BookListModel initialized with" << _books.size() << "books";
}

int BookListModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return static_cast<int>(_books.size());
}

QVariant BookListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= _books.size())
    return {};

  const auto &book = _books.at(index.row());

  switch (role) {
  case IdRole:
    return book.id;
  case NameRole:
    return book.name;
  case AuthorIdRole:
    return book.authorId;
  case AuthorRole:
    return book.authorName;
  case YearRole:
    return book.year;
  case PublisherIdRole:
    return book.publisherId;
  case PublisherRole:
    return book.publisherName;
  case DescriptionRole:
    return book.description;
  case CoverUrlRole:
    return book.coverUrl;
  case IsHardcoverRole:
    return book.isHardcover;
  case TypeIdRole:
    return book.typeId;
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

QHash<int, QByteArray> BookListModel::roleNames() const {
  return {
      {IdRole, "bookId"},
      {NameRole, "name"},
      {AuthorIdRole, "authorId"},
      {AuthorRole, "author"},
      {YearRole, "year"},
      {PublisherIdRole, "publisherId"},
      {PublisherRole, "publisher"},
      {DescriptionRole, "description"},
      {CoverUrlRole, "coverUrl"},
      {IsHardcoverRole, "isHardcover"},
      {TypeIdRole, "typeId"},
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

bool BookListModel::deleteBook(qint64 id) {
  if (!_bookTable->deleteBook(id)) {
    setErrorMessage(tr("Failed to delete book."));
    return false;
  }

  qCInfo(lcBookModel) << "Book deleted — id:" << id;
  setErrorMessage({});
  refresh();
  return true;
}

QVariantMap BookListModel::getBook(qint64 id) const {
  auto it = std::find_if(_books.begin(), _books.end(), [id](const services::BookDTO &book) { return book.id == id; });
  if (it != _books.end()) {
    return it->toMap();
  }
  qCWarning(lcBookModel) << "getBook — book not found, id:" << id;
  return {};
}

void BookListModel::refresh() {
  beginResetModel();
  _books = _bookTable->getAllBooks();
  endResetModel();
}

QString BookListModel::errorMessage() const { return _errorMessage; }

void BookListModel::setErrorMessage(const QString &message) {
  if (_errorMessage == message)
    return;
  _errorMessage = message;
  emit errorMessageChanged();
}

} // namespace bl::models
