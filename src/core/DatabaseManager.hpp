#ifndef READARY_CORE_DATABASEMANAGER_HPP
#define READARY_CORE_DATABASEMANAGER_HPP

#include "SqlQueryBuilder.hpp"

#include <QString>
#include <QtSql/QSqlDatabase>

namespace readary::core {

class DatabaseManager {
public:
  explicit DatabaseManager(const QString &dbName);
  ~DatabaseManager();

  DatabaseManager(const DatabaseManager &) = delete;
  DatabaseManager &operator=(const DatabaseManager &) = delete;

  bool open();
  void close();
  bool runScript(const QString &scriptFileName);
  bool clear(QString *error = nullptr);
  bool exec(const QString &sql, QString *error = nullptr);
  QList<QVariantMap> select(const SqlQueryBuilder &builder, QString *error = nullptr);
  int execute(const SqlQueryBuilder &builder, QString *error = nullptr);
  qint64 insert(const SqlQueryBuilder &builder, QString *error = nullptr);

private:
  static bool execPrepared(QSqlQuery &query, const SqlQueryBuilder &builder, QString *error);
  void trimRun(QTextStream &script);
  static QList<QVariantMap> getDataFromQuery(QSqlQuery &query);

  static constexpr QLatin1StringView kConnectionName{"Readary"};

  QSqlDatabase _db;
};

} // namespace readary::core

#endif // READARY_CORE_DATABASEMANAGER_HPP
