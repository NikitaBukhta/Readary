#ifndef BEELIBRARY_CORE_SQLQUERYBUILDER_HPP
#define BEELIBRARY_CORE_SQLQUERYBUILDER_HPP

#include <QString>
#include <QStringList>
#include <QVariantList>

namespace bl::core {

class SqlQueryBuilder {
public:
  SqlQueryBuilder &select(const QStringList &columns = {});
  SqlQueryBuilder &selectCount();
  SqlQueryBuilder &insertInto(const QString &table, const QStringList &columns);
  SqlQueryBuilder &update(const QString &table);
  SqlQueryBuilder &from(const QString &table, const QString &alias = "");
  SqlQueryBuilder &deleteFrom(const QString &table);
  SqlQueryBuilder &where(const QString &condition);
  SqlQueryBuilder &set(const QStringList &columns);
  SqlQueryBuilder &leftJoin(const QString &table, const QString &alias = "");
  SqlQueryBuilder &on(const QString &condition);
  SqlQueryBuilder &orderBy(const QString &column, const QString &order = "ASC");
  SqlQueryBuilder &values(const QVariantList &values);
  SqlQueryBuilder &values(QVariantList &&values);

  const QVariantList &getValues() const;

  QString build() const;

private:
  QString _query;
  QVariantList _values;
};

} // namespace bl::core

#endif // BEELIBRARY_CORE_SQLQUERYBUILDER_HPP
