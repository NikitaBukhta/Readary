#ifndef BEELIBRARY_MODELS_BOOKCRITERIAFILTERPROXYMODEL_HPP
#define BEELIBRARY_MODELS_BOOKCRITERIAFILTERPROXYMODEL_HPP

#include "services/BookFilterCriteria.hpp"

#include <QSortFilterProxyModel>
#include <QtQml/qqmlregistration.h>

namespace readary::models {

class BookCriteriaFilterProxyModel : public QSortFilterProxyModel {
  Q_OBJECT
  QML_ANONYMOUS

  Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
  explicit BookCriteriaFilterProxyModel(QObject *parent = nullptr);

  const services::BookFilterCriteria &criteria() const;
  void setCriteria(const services::BookFilterCriteria &criteria);
  void clearCriteria();

signals:
  void countChanged();

protected:
  bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
  services::BookFilterCriteria _criteria;
};

} // namespace readary::models

#endif // BEELIBRARY_MODELS_BOOKCRITERIAFILTERPROXYMODEL_HPP
