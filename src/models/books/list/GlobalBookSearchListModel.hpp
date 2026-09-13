#ifndef READARY_MODELS_BOOKS_GLOBALBOOKSEARCHLISTMODEL_HPP
#define READARY_MODELS_BOOKS_GLOBALBOOKSEARCHLISTMODEL_HPP

#include "models/books/list/BookListModelBase.hpp"

#include <QtQml/qqmlregistration.h>

namespace readary::models {

class GlobalBookSearchListModel : public BookListModelBase {
  Q_OBJECT
  QML_ANONYMOUS
public:
  explicit GlobalBookSearchListModel(QObject *parent = nullptr);

  void refresh() override;
};

} // namespace readary::models

#endif // READARY_MODELS_BOOKS_GLOBALBOOKSEARCHLISTMODEL_HPP
