#ifndef READARY_MODELS_BOOKS_BOOKLISTMODEL_HPP
#define READARY_MODELS_BOOKS_BOOKLISTMODEL_HPP

#include "models/books/list/BookListModelBase.hpp"
#include "services/dto/BookDTO.hpp"
#include "services/storage/BookTable.hpp"

#include <memory>

namespace readary::models {

class BookListModel : public BookListModelBase {
  Q_OBJECT

  Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
  explicit BookListModel(std::shared_ptr<services::BookTable> bookTable, QObject *parent);
  void refresh() override;

  QString errorMessage() const;

  Q_INVOKABLE bool deleteBook(qint64 isbn);

signals:
  void errorMessageChanged();

private:
  void setErrorMessage(const QString &message);

  std::shared_ptr<services::BookTable> _bookTable;
  QString _errorMessage;
};

} // namespace readary::models

#endif // READARY_MODELS_BOOKS_BOOKLISTMODEL_HPP
