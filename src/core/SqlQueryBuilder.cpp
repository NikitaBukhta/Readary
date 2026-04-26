#include "SqlQueryBuilder.hpp"

namespace bl::core {

SqlQueryBuilder &SqlQueryBuilder::select(const QStringList &columns) {
  if (columns.isEmpty() ||
      (columns.size() == 1 && columns.first().trimmed() == "*")) {
    _query = "SELECT *";
  } else {
    _query = "SELECT " + columns.join(", ");
  }
  return *this;
}

SqlQueryBuilder &SqlQueryBuilder::selectCount() {
  _query = "SELECT COUNT(*)";
  return *this;
}

SqlQueryBuilder &SqlQueryBuilder::insertInto(const QString &table,
                                             const QStringList &columns) {
  _query += "INSERT INTO " + table + " (" + columns.join(", ") + ") VALUES (" +
            QString(", ?").repeated(columns.size()).mid(2) + ")";
  _values.clear();
  return *this;
}

SqlQueryBuilder &SqlQueryBuilder::update(const QString &table) {
  _query += "UPDATE " + table;
  return *this;
}

SqlQueryBuilder &SqlQueryBuilder::from(const QString &table,
                                       const QString &alias) {
  _query += " FROM " + table;
  if (!alias.isEmpty())
    _query += " " + alias;
  return *this;
}

SqlQueryBuilder &SqlQueryBuilder::deleteFrom(const QString &table) {
  _query += "DELETE FROM " + table;
  return *this;
}

SqlQueryBuilder &SqlQueryBuilder::where(const QString &condition) {
  _query += " WHERE " + condition;
  return *this;
}

SqlQueryBuilder &SqlQueryBuilder::set(const QStringList &columns) {
  _query += " SET " + columns.join(" = ?, ") + " = ?";
  return *this;
}

SqlQueryBuilder &SqlQueryBuilder::leftJoin(const QString &table,
                                           const QString &alias) {
  _query += " LEFT JOIN " + table;
  if (!alias.isEmpty())
    _query += " " + alias;
  return *this;
}

SqlQueryBuilder &SqlQueryBuilder::on(const QString &condition) {
  _query += " ON " + condition;
  return *this;
}

SqlQueryBuilder &SqlQueryBuilder::orderBy(const QString &column,
                                          const QString &order) {
  _query += " ORDER BY " + column + " " + order;
  return *this;
}

SqlQueryBuilder &SqlQueryBuilder::values(const QVariantList &values) {
  _values = values;
  return *this;
}

SqlQueryBuilder &SqlQueryBuilder::values(QVariantList &&values) {
  _values = std::move(values);
  return *this;
}

const QVariantList &SqlQueryBuilder::getValues() const { return _values; }

QString SqlQueryBuilder::build() const { return _query; }

} // namespace bl::core
