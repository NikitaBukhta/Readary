#ifndef LIBRARY_GLOBALBOOKSEARCHLISTMODEL_HPP
#define LIBRARY_GLOBALBOOKSEARCHLISTMODEL_HPP

#include "BookListModelBase.hpp"

#include <QtQml/qqmlregistration.h>

namespace readary {
namespace models {

class GlobalBookSearchListModel : public BookListModelBase {
  Q_OBJECT
  QML_ANONYMOUS
public:
  explicit GlobalBookSearchListModel(QObject *parent = nullptr);

  void refresh() override;

public slots:
  void onSearchListUpdated(const QList<services::BookDTO> &books);
};

} // namespace models
} // namespace readary

#endif // LIBRARY_GLOBALBOOKSEARCHLISTMODEL_HPP
