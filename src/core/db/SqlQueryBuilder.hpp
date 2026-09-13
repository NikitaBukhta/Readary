#ifndef READARY_CORE_SQLQUERYBUILDER_HPP
#define READARY_CORE_SQLQUERYBUILDER_HPP

#include <QString>
#include <QStringList>
#include <QVariantList>

namespace readary::core {

class SqlQueryBuilder {
public:
  SqlQueryBuilder &select(const QStringList &columns = {});
  SqlQueryBuilder &selectCount();
  SqlQueryBuilder &insertInto(const QString &table, const QStringList &columns);
  SqlQueryBuilder &insertOrIgnoreInto(const QString &table, const QStringList &columns);
  SqlQueryBuilder &update(const QString &table);
  SqlQueryBuilder &from(const QString &table, const QString &alias = {});
  SqlQueryBuilder &deleteFrom(const QString &table);
  SqlQueryBuilder &where(const QString &condition);
  SqlQueryBuilder &set(const QStringList &columns);
  SqlQueryBuilder &leftJoin(const QString &table, const QString &alias = {});
  SqlQueryBuilder &on(const QString &condition);
  SqlQueryBuilder &orderBy(const QString &column, const QString &order = "ASC");
  SqlQueryBuilder &values(const QVariantList &values);
  SqlQueryBuilder &values(QVariantList &&values);

  const QVariantList &getValues() const;

  QString build() const;

private:
  SqlQueryBuilder &appendInsert(const QString &verb, const QString &table, const QStringList &columns);

  QString _query;
  QVariantList _values;
  bool _hasOrderBy = false;
};

} // namespace readary::core

#endif // READARY_CORE_SQLQUERYBUILDER_HPP
