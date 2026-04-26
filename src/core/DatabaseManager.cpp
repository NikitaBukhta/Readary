#include "DatabaseManager.hpp"

#include <QFile>
#include <QLoggingCategory>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariantMap>

Q_LOGGING_CATEGORY(lcDb, "bl.core.db")

namespace bl::core {

DatabaseManager::DatabaseManager(const QString &dbName) {
  _db = QSqlDatabase::addDatabase("QSQLITE", kConnectionName);
  _db.setDatabaseName(dbName);
  qCInfo(lcDb) << "Database configured:" << dbName;
}

DatabaseManager::~DatabaseManager() {
  close();
  _db = QSqlDatabase{};
  QSqlDatabase::removeDatabase(kConnectionName);
}

bool DatabaseManager::open() {
  if (_db.open()) {
    qCInfo(lcDb) << "Database opened:" << _db.databaseName();
    return true;
  }

  qCWarning(lcDb) << "Failed to open database:" << _db.lastError().text();
  return false;
}

void DatabaseManager::close() {
  if (_db.isOpen()) {
    qCInfo(lcDb) << "Database closed";
    _db.close();
  }
}

bool DatabaseManager::runScript(const QString &scriptFileName) {
  QFile file(scriptFileName);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qCWarning(lcDb) << "Cannot open script" << scriptFileName << ":"
                    << file.errorString();
    return false;
  }

  qCInfo(lcDb) << "Running script:" << scriptFileName;

  QTextStream script(&file);
  trimRun(script);

  return true;
}

QList<QVariantMap> DatabaseManager::select(const SqlQueryBuilder &builder,
                                           QString *error) {
  QSqlQuery query{_db};

  if (!execPrepared(query, builder, error))
    return {};

  return getDataFromQuery(query);
}

int DatabaseManager::execute(const SqlQueryBuilder &builder, QString *error) {
  QSqlQuery query{_db};

  if (!_db.transaction()) {
    qCWarning(lcDb) << "Failed to begin transaction:" << _db.lastError().text();
    if (error)
      *error = _db.lastError().text();
    return -1;
  }

  if (!execPrepared(query, builder, error)) {
    _db.rollback();
    return -1;
  }

  _db.commit();
  return query.numRowsAffected();
}

qint64 DatabaseManager::insert(const SqlQueryBuilder &builder, QString *error) {
  QSqlQuery query{_db};

  if (!_db.transaction()) {
    qCWarning(lcDb) << "Failed to begin transaction:" << _db.lastError().text();
    if (error)
      *error = _db.lastError().text();
    return -1;
  }

  if (!execPrepared(query, builder, error)) {
    _db.rollback();
    return -1;
  }

  _db.commit();

  auto lastId = query.lastInsertId();
  return lastId.isValid() ? lastId.toLongLong() : 0;
}

bool DatabaseManager::exec(const QString &sql, QString *error) {
  QSqlQuery query{_db};

  if (!_db.transaction()) {
    qCWarning(lcDb) << "Failed to begin transaction:" << _db.lastError().text();
    if (error)
      *error = _db.lastError().text();
    return false;
  }

  if (!query.exec(sql)) {
    qCWarning(lcDb) << "Query failed:" << query.lastError().text();
    if (error)
      *error = query.lastError().text();
    _db.rollback();
    return false;
  }

  _db.commit();
  return true;
}

bool DatabaseManager::execPrepared(QSqlQuery &query,
                                   const SqlQueryBuilder &builder,
                                   QString *error) {
  const auto &params = builder.getValues();
  QString sql = builder.build();

  if (!query.prepare(sql)) {
    qCWarning(lcDb) << "Prepare failed:" << query.lastError().text()
                    << "sql:" << sql;
    if (error)
      *error = query.lastError().text();
    return false;
  }

  for (const auto &param : params)
    query.addBindValue(param);

  qCDebug(lcDb) << "Executing:" << sql << "params:" << params;

  if (!query.exec()) {
    qCWarning(lcDb) << "Query failed:" << query.lastError().text();
    if (error)
      *error = query.lastError().text();
    return false;
  }

  return true;
}

void DatabaseManager::trimRun(QTextStream &script) {
  QString currentCommand;

  while (!script.atEnd()) {
    QString line = script.readLine();
    if (line.isEmpty() || line.startsWith("--"))
      continue;

    if (line.startsWith(" ")) {
      auto trimmedLine = line.trimmed();
      auto endIndex = trimmedLine.indexOf("--");
      if (endIndex != -1)
        currentCommand += trimmedLine.left(endIndex) + " ";
      else
        currentCommand += trimmedLine + " ";
    } else {
      currentCommand += line + " ";
      if (line.trimmed().endsWith(";")) {
        QString error;
        if (!exec(currentCommand, &error))
          qCWarning(lcDb) << "Script statement failed:" << error;
        currentCommand.clear();
      }
    }
  }
}

QList<QVariantMap> DatabaseManager::getDataFromQuery(QSqlQuery &query) {
  QList<QVariantMap> ret;
  const QSqlRecord schema = query.record();
  const int fieldCount = schema.count();

  while (query.next()) {
    QVariantMap row;
    for (int i = 0; i < fieldCount; ++i)
      row.insert(schema.fieldName(i), query.value(i));
    ret.emplaceBack(std::move(row));
  }

  qCDebug(lcDb) << "Query returned" << ret.size() << "rows";
  return ret;
}

} // namespace bl::core
