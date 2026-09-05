#include "SqlQueryBuilder.hpp"

using Qt::StringLiterals::operator""_s;

namespace readary::core {

SqlQueryBuilder &SqlQueryBuilder::select(const QStringList &columns) {
  if (columns.isEmpty() || (columns.size() == 1 && columns.first().trimmed() == u"*"_s)) {
    _query = u"SELECT *"_s;
  } else {
    _query = u"SELECT "_s + columns.join(u", "_s);
  }
  return *this;
}

SqlQueryBuilder &SqlQueryBuilder::selectCount() {
  _query = u"SELECT COUNT(*)"_s;
  return *this;
}

SqlQueryBuilder &SqlQueryBuilder::insertInto(const QString &table, const QStringList &columns) {
  return appendInsert(u"INSERT INTO "_s, table, columns);
}

SqlQueryBuilder &SqlQueryBuilder::insertOrIgnoreInto(const QString &table, const QStringList &columns) {
  return appendInsert(u"INSERT OR IGNORE INTO "_s, table, columns);
}

SqlQueryBuilder &SqlQueryBuilder::appendInsert(const QString &verb, const QString &table, const QStringList &columns) {
  const QString placeholders = QStringList(columns.size(), u"?"_s).join(u", "_s);
  _query += verb + table + u" ("_s + columns.join(u", "_s) + u") VALUES ("_s + placeholders + u")"_s;
  _values.clear();
  return *this;
}

SqlQueryBuilder &SqlQueryBuilder::update(const QString &table) {
  _query += u"UPDATE "_s + table;
  return *this;
}

SqlQueryBuilder &SqlQueryBuilder::from(const QString &table, const QString &alias) {
  _query += u" FROM "_s + table;
  if (!alias.isEmpty()) {
    _query += u" "_s + alias;
  }
  return *this;
}

SqlQueryBuilder &SqlQueryBuilder::deleteFrom(const QString &table) {
  _query += u"DELETE FROM "_s + table;
  return *this;
}

SqlQueryBuilder &SqlQueryBuilder::where(const QString &condition) {
  _query += u" WHERE "_s + condition;
  return *this;
}

SqlQueryBuilder &SqlQueryBuilder::set(const QStringList &columns) {
  _query += u" SET "_s + columns.join(u" = ?, "_s) + u" = ?"_s;
  return *this;
}

SqlQueryBuilder &SqlQueryBuilder::leftJoin(const QString &table, const QString &alias) {
  _query += u" LEFT JOIN "_s + table;
  if (!alias.isEmpty()) {
    _query += u" "_s + alias;
  }
  return *this;
}

SqlQueryBuilder &SqlQueryBuilder::on(const QString &condition) {
  _query += u" ON "_s + condition;
  return *this;
}

SqlQueryBuilder &SqlQueryBuilder::orderBy(const QString &column, const QString &order) {
  // Chained calls extend the same clause. Tracked in a flag rather than searched
  // for in the query text, which a subquery's own ORDER BY would fool.
  _query += (_hasOrderBy ? u", "_s : u" ORDER BY "_s) + column + u" "_s + order;
  _hasOrderBy = true;
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

} // namespace readary::core
