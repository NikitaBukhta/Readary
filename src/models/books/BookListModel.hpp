#ifndef BEELIBRARY_MODELS_BOOKLISTMODEL_HPP
#define BEELIBRARY_MODELS_BOOKLISTMODEL_HPP

#include "BookListModelBase.hpp"
#include "services/BookDTO.hpp"
#include "services/BookTable.hpp"

#include <QAbstractListModel>
#include <QList>
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

private:
  std::shared_ptr<services::BookTable> _bookTable;
  QString _errorMessage;
};

} // namespace readary::models

#endif // BEELIBRARY_MODELS_BOOKLISTMODEL_HPP
