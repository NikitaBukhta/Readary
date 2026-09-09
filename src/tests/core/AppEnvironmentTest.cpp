#include "core/AppEnvironment.hpp"

#include <QStandardPaths>
#include <QTest>

using Qt::StringLiterals::operator""_s;

using readary::core::AppEnvironment;

class AppEnvironmentTest : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void cleanup();

  void dataPath_isAnExistingDirectory();
  void dataPath_isNamedAfterTheApp();
  void dataPath_isStableAcrossCalls();
  void databasePath_livesInTheDataDirectory();
  void logFilePath_livesInTheDataDirectory();
  void logFilePath_isStampedPerCall();
  void installFileLogger_startsWritingToDisk();
  void shutdownFileLogger_stopsWriting();
  void shutdownFileLogger_withoutAnInstall_isANoOp();
};

void AppEnvironmentTest::initTestCase() {
  // Keeps the data directory inside the test sandbox, not the real user profile.
  QStandardPaths::setTestModeEnabled(true);
  QCoreApplication::setOrganizationName("DarieszzBooksTests");
  QCoreApplication::setOrganizationDomain("tests.darieszzbooks.local");
  QCoreApplication::setApplicationName("AppEnvironmentTest");
}

void AppEnvironmentTest::cleanup() { AppEnvironment::shutdownFileLogger(); }

void AppEnvironmentTest::dataPath_isAnExistingDirectory() {
  const QString path = AppEnvironment::dataPath();

  QVERIFY(!path.isEmpty());
  QVERIFY(QDir{path}.exists());
}

void AppEnvironmentTest::dataPath_isNamedAfterTheApp() {
  // One "Readary" directory beside the platform's app-data root, shared by
  // every build of the app rather than one per application name.
  QCOMPARE(QFileInfo{AppEnvironment::dataPath()}.fileName(), u"Readary"_s);
}

void AppEnvironmentTest::dataPath_isStableAcrossCalls() {
  QCOMPARE(AppEnvironment::dataPath(), AppEnvironment::dataPath());
}

void AppEnvironmentTest::databasePath_livesInTheDataDirectory() {
  const QString path = AppEnvironment::databasePath();

  QCOMPARE(QFileInfo{path}.fileName(), u"readary.db"_s);
  QCOMPARE(QFileInfo{path}.absolutePath(), QDir{AppEnvironment::dataPath()}.absolutePath());
}

void AppEnvironmentTest::logFilePath_livesInTheDataDirectory() {
  const QString path = AppEnvironment::logFilePath();

  QVERIFY(QFileInfo{path}.fileName().startsWith(u"log_"_s));
  QVERIFY(path.endsWith(u".log"_s));
  QCOMPARE(QFileInfo{path}.absolutePath(), QDir{AppEnvironment::dataPath()}.absolutePath());
}

void AppEnvironmentTest::logFilePath_isStampedPerCall() {
  // cleanupOldLogs() finds yesterday's logs by this exact glob and the day
  // stamp is what makes one run's log distinguishable from the next.
  const QString name = QFileInfo{AppEnvironment::logFilePath()}.fileName();

  const QRegularExpression stamped{uR"(^log_\d{2}\.\d{2}\.\d{4}-\d{2}\.\d{2}\.\d{2}\.log$)"_s};
  QVERIFY2(stamped.match(name).hasMatch(), qPrintable(name));
}

void AppEnvironmentTest::installFileLogger_startsWritingToDisk() {
  AppEnvironment::installFileLogger();

  qWarning() << "readary-test-marker";
  AppEnvironment::shutdownFileLogger();

  const QDir dir{AppEnvironment::dataPath()};
  const auto logs = dir.entryInfoList({u"log_*.log"_s}, QDir::Files, QDir::Time);
  QVERIFY(!logs.isEmpty());

  QFile newest{logs.first().absoluteFilePath()};
  QVERIFY(newest.open(QIODevice::ReadOnly | QIODevice::Text));
  QVERIFY(QString::fromUtf8(newest.readAll()).contains(u"readary-test-marker"_s));
}

void AppEnvironmentTest::shutdownFileLogger_stopsWriting() {
  AppEnvironment::installFileLogger();
  AppEnvironment::shutdownFileLogger();

  const QDir dir{AppEnvironment::dataPath()};
  const auto before = dir.entryInfoList({u"log_*.log"_s}, QDir::Files, QDir::Time);
  QVERIFY(!before.isEmpty());
  const qint64 sizeBefore = before.first().size();

  qWarning() << "readary-test-after-shutdown";

  const auto after = dir.entryInfoList({u"log_*.log"_s}, QDir::Files, QDir::Time);
  QCOMPARE(after.first().size(), sizeBefore);
}

void AppEnvironmentTest::shutdownFileLogger_withoutAnInstall_isANoOp() {
  AppEnvironment::shutdownFileLogger();
  AppEnvironment::shutdownFileLogger();
}

QTEST_GUILESS_MAIN(AppEnvironmentTest)
#include "AppEnvironmentTest.moc"
