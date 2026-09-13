#include "AppEnvironment.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QTextStream>
#include <QtGlobal>

#include <iostream>
#include <memory>

#ifdef Q_OS_ANDROID
#include <android/log.h>
#endif

using Qt::StringLiterals::operator""_L1;
using Qt::StringLiterals::operator""_s;

namespace {
Q_LOGGING_CATEGORY(lcAppEnv, "readary.core.env")

constexpr auto g_appDirName = "Readary"_L1;
constexpr auto g_databaseFileName = "/readary.db"_L1;
constexpr auto g_bookFilesDirName = "/books"_L1;
constexpr auto g_logFilePattern = "log_*.log"_L1;

std::unique_ptr<QFile> g_logFile;

QMutex &logMutex() {
  static QMutex mutex;
  return mutex;
}
} // namespace

namespace readary::core {

QString AppEnvironment::ensureDataDir() {
  QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QDir dir(path);
  dir.cdUp();
  path = dir.absoluteFilePath(g_appDirName);

  if (!QDir().mkpath(path)) {
    qCWarning(lcAppEnv) << "Cannot create data directory:" << path;
  }
  return path;
}

QString AppEnvironment::dataPath() {
  static const QString path = ensureDataDir();
  return path;
}

QString AppEnvironment::databasePath() { return dataPath() + g_databaseFileName; }
QString AppEnvironment::bookFilesPath() { return dataPath() + g_bookFilesDirName; }

QString AppEnvironment::logFilePath() {
  const QString timestamp = QDateTime::currentDateTime().toString(u"dd.MM.yyyy-hh.mm.ss"_s);
  return dataPath() + u"/log_"_s + timestamp + u".log"_s;
}

void AppEnvironment::installFileLogger() {
  cleanupOldLogs();

  const QString path = logFilePath();

  auto logFile = std::make_unique<QFile>(path);
  if (!logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
    qCWarning(lcAppEnv) << "Cannot open log file:" << path;
    return;
  }

  g_logFile = std::move(logFile);
  qInstallMessageHandler(messageHandler);
}

void AppEnvironment::shutdownFileLogger() {
  qInstallMessageHandler(nullptr);

  const QMutexLocker locker(&logMutex());
  if (g_logFile != nullptr) {
    g_logFile->flush();
    g_logFile->close();
    g_logFile = nullptr;
  }
}

void AppEnvironment::cleanupOldLogs(int keepDays) {
  const QDir dir(dataPath());
  const QDateTime cutoff = QDateTime::currentDateTime().addDays(-keepDays);

  const auto entries = dir.entryInfoList({g_logFilePattern}, QDir::Files, QDir::Time);
  for (const QFileInfo &info : entries) {
    if (info.lastModified() < cutoff && QFile::remove(info.absoluteFilePath())) {
      qCInfo(lcAppEnv) << "Removed old log:" << info.fileName();
    }
  }
}

void AppEnvironment::messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
  QLatin1StringView level;
  switch (type) {
  case QtDebugMsg:
    level = "DEBUG"_L1;
    break;
  case QtInfoMsg:
    level = "INFO "_L1;
    break;
  case QtWarningMsg:
    level = "WARN "_L1;
    break;
  case QtCriticalMsg:
    level = "CRIT "_L1;
    break;
  case QtFatalMsg:
    level = "FATAL"_L1;
    break;
  }

  const QString timestamp = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
  const QString category = context.category != nullptr ? QString::fromUtf8(context.category) : u"default"_s;

  const QString line = u"%1 [%2] %3: %4\n"_s.arg(timestamp, level, category, msg);

  if (g_logFile != nullptr) {
    const QMutexLocker locker(&logMutex());
    QTextStream stream(g_logFile.get());
    stream << line;
    stream.flush();
  }

#ifdef Q_OS_ANDROID
  // stderr is dropped on Android; logcat is the only practical channel.
  // Single tag "Readary" simplifies `adb logcat *:S Readary:V` filtering.
  android_LogPriority prio = ANDROID_LOG_INFO;
  switch (type) {
  case QtDebugMsg:
    prio = ANDROID_LOG_DEBUG;
    break;
  case QtInfoMsg:
    prio = ANDROID_LOG_INFO;
    break;
  case QtWarningMsg:
    prio = ANDROID_LOG_WARN;
    break;
  case QtCriticalMsg:
    prio = ANDROID_LOG_ERROR;
    break;
  case QtFatalMsg:
    prio = ANDROID_LOG_FATAL;
    break;
  }
  const QString body = u"[%1] %2"_s.arg(category, msg);
  __android_log_write(prio, "Readary", body.toUtf8().constData());
#elif !defined(QT_NO_DEBUG)
  std::cerr << line.toLocal8Bit().constData();
#endif
}

} // namespace readary::core
