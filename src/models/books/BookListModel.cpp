#include "BookListModel.hpp"

#include <QLoggingCategory>

namespace {
Q_LOGGING_CATEGORY(lcBookModel, "readary.models.books")
}

namespace readary::models {

BookListModel::BookListModel(std::shared_ptr<services::BookTable> bookTable, QObject *parent)
    : BookListModelBase(parent), _bookTable{std::move(bookTable)} {
}

bool BookListModel::deleteBook(qint64 isbn) {
  if (!_bookTable->deleteBook(isbn)) {
    setErrorMessage(tr("Failed to delete book."));
    return false;
  }

  qCInfo(lcBookModel) << "Book deleted — isbn:" << isbn;
  setErrorMessage({});
  refresh();
  return true;
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

} // namespace readary::models
