#ifndef READARY_CORE_APPENVIRONMENT_HPP
#define READARY_CORE_APPENVIRONMENT_HPP

#include <QString>

namespace readary::core {

class AppEnvironment {
public:
  static QString dataPath();
  static QString databasePath();
  static QString bookFilesPath();
  static QString logFilePath();

  static void installFileLogger();
  static void shutdownFileLogger();

private:
  static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

  static QString ensureDataDir();
  static void cleanupOldLogs(int keepDays = 7);
};

} // namespace readary::core

#endif // READARY_CORE_APPENVIRONMENT_HPP
