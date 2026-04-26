#ifndef BEELIBRARY_CORE_DATABASEMANAGER_HPP
#define BEELIBRARY_CORE_DATABASEMANAGER_HPP

#include "SqlQueryBuilder.hpp"

#include <QString>
#include <QtSql/QSqlDatabase>

namespace bl::core {

class DatabaseManager {
public:
  explicit DatabaseManager(const QString &dbName);
  ~DatabaseManager();

  DatabaseManager(const DatabaseManager &) = delete;
  DatabaseManager &operator=(const DatabaseManager &) = delete;

  bool open();
  void close();
  bool runScript(const QString &scriptFileName);

  bool exec(const QString &sql, QString *error = nullptr);
  QList<QVariantMap> select(const SqlQueryBuilder &query,
                            QString *error = nullptr);
  int execute(const SqlQueryBuilder &query, QString *error = nullptr);
  qint64 insert(const SqlQueryBuilder &query, QString *error = nullptr);

private:
  bool execPrepared(QSqlQuery &query, const SqlQueryBuilder &builder,
                    QString *error);
  void trimRun(QTextStream &script);
  QList<QVariantMap> getDataFromQuery(QSqlQuery &query);

  static constexpr const char *kConnectionName = "BeeLibrary";

  QSqlDatabase _db;
};

} // namespace bl::core

#endif // BEELIBRARY_CORE_DATABASEMANAGER_HPP
