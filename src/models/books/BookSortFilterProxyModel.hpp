#ifndef READARY_MODELS_BOOKS_BOOKSORTFILTERPROXYMODEL_HPP
#define READARY_MODELS_BOOKS_BOOKSORTFILTERPROXYMODEL_HPP

#include <QMultiHash>
#include <QSortFilterProxyModel>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

#include <cstdint>

namespace readary::models {

class BookSortFilterProxyModel : public QSortFilterProxyModel {
  Q_OBJECT
  QML_ANONYMOUS

  Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
  Q_PROPERTY(int sortField READ sortRole WRITE setSortField NOTIFY sortFieldChanged)
  Q_PROPERTY(bool sortDescending READ sortDescending WRITE setSortDescending NOTIFY sortDescendingChanged)

public:
  enum class Op : std::uint8_t {
    Equal,
    NotEqual,
    Less,
    LessOrEqual,
    Greater,
    GreaterOrEqual,
    Contains,
  };
  Q_ENUM(Op)

  explicit BookSortFilterProxyModel(QObject *parent = nullptr);

  void addFilter(int role, const QVariant &value, Op op = Op::Equal);
  void removeFilter(int role);
  void clearFilter();

  void setSortField(int role);

  bool sortDescending() const;
  void setSortDescending(bool descending);

signals:
  void countChanged();
  void sortFieldChanged();
  void sortDescendingChanged();

protected:
  bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
  bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
  struct Filter {
    QVariant value;
    Op op;
  };

  static bool matches(const QVariant &cell, const Filter &filter);

  QMultiHash<int, Filter> _filters;
};

} // namespace readary::models

#endif // READARY_MODELS_BOOKS_BOOKSORTFILTERPROXYMODEL_HPP
