#include "AppEnvironment.hpp"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QTextStream>

#include <iostream>

namespace {
Q_LOGGING_CATEGORY(lcAppEnv, "bl.core.env")

QFile *g_logFile = nullptr;
QMutex g_logMutex;
} // namespace

namespace bl::core {

QString AppEnvironment::ensureDataDir() {
  QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QDir dir(path);
  dir.cdUp();
  path = dir.absoluteFilePath("BeeLibrary");

  QDir().mkpath(path);
  return path;
}

QString AppEnvironment::dataPath() {
  static const QString path = ensureDataDir();
  return path;
}

QString AppEnvironment::databasePath() { return dataPath() + "/beelibrary.db"; }

QString AppEnvironment::logFilePath() {
  const QString timestamp = QDateTime::currentDateTime().toString("dd.MM.yyyy-hh.mm.ss");
  return dataPath() + "/log_" + timestamp + ".log";
}

void AppEnvironment::installFileLogger() {
  cleanupOldLogs();

  const QString path = logFilePath();

  g_logFile = new QFile(path);
  if (!g_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
    delete g_logFile;
    g_logFile = nullptr;
    qCWarning(lcAppEnv) << "Cannot open log file:" << path;
    return;
  }

  qInstallMessageHandler(messageHandler);
}

void AppEnvironment::shutdownFileLogger() {
  qInstallMessageHandler(nullptr);

  const QMutexLocker locker(&g_logMutex);
  if (g_logFile) {
    g_logFile->flush();
    g_logFile->close();
    delete g_logFile;
    g_logFile = nullptr;
  }
}

void AppEnvironment::cleanupOldLogs(int keepDays) {
  const QDir dir(dataPath());
  const QDateTime cutoff = QDateTime::currentDateTime().addDays(-keepDays);

  const auto entries = dir.entryInfoList({"log_*.log"}, QDir::Files, QDir::Time);
  for (const QFileInfo &info : entries) {
    if (info.lastModified() < cutoff) {
      if (QFile::remove(info.absoluteFilePath()))
        qCInfo(lcAppEnv) << "Removed old log:" << info.fileName();
    }
  }
}

void AppEnvironment::messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
  if (!g_logFile)
    return;

  const char *level = nullptr;
  switch (type) {
  case QtDebugMsg:
    level = "DEBUG";
    break;
  case QtInfoMsg:
    level = "INFO ";
    break;
  case QtWarningMsg:
    level = "WARN ";
    break;
  case QtCriticalMsg:
    level = "CRIT ";
    break;
  case QtFatalMsg:
    level = "FATAL";
    break;
  }

  const QString timestamp = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
  const QString category = context.category ? context.category : "default";

  const QString line = QStringLiteral("%1 [%2] %3: %4\n").arg(timestamp, level, category, msg);

  const QMutexLocker locker(&g_logMutex);
  QTextStream stream(g_logFile);
  stream << line;
  stream.flush();

#ifndef QT_NO_DEBUG
  std::cerr << line.toLocal8Bit().constData();
#endif
}

} // namespace bl::core
