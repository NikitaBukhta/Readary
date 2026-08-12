#include "services/ReadingSessionCache.hpp"
#include "services/ReadingPhase.hpp"

#include <QSettings>
#include <QStandardPaths>
#include <QTest>

using Qt::StringLiterals::operator""_s;

using readary::services::ReadingPhase;
using readary::services::ReadingSessionCache;

namespace {

constexpr qint64 kIsbn{9780201616224LL};

} // namespace

class ReadingSessionCacheTest : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void init();

  void takeState_missing_returnsEmpty();
  void save_thenTake_roundTripsPausedState();
  void take_removesEntry_soSecondTakeIsEmpty();
  void save_runningPhase_addsElapsedWallClock();
  void clear_removesEntry();
  void save_ignoresNonPositiveIsbn();
};

void ReadingSessionCacheTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  QCoreApplication::setOrganizationName("DarieszzBooksTests");
  QCoreApplication::setOrganizationDomain("tests.darieszzbooks.local");
  QCoreApplication::setApplicationName("ReadingSessionCacheTest");
}

void ReadingSessionCacheTest::init() {
  QSettings settings;
  settings.clear();
  settings.sync();
}

void ReadingSessionCacheTest::takeState_missing_returnsEmpty() {
  QVERIFY(ReadingSessionCache::takeState(kIsbn).isEmpty());
}

void ReadingSessionCacheTest::save_thenTake_roundTripsPausedState() {
  ReadingSessionCache::save(kIsbn, 125, ReadingPhase::Paused);

  const QVariantMap out = ReadingSessionCache::takeState(kIsbn);
  QCOMPARE(out.value("seconds").toInt(), 125);
  QCOMPARE(out.value("phase").toInt(), static_cast<int>(ReadingPhase::Paused));
}

void ReadingSessionCacheTest::take_removesEntry_soSecondTakeIsEmpty() {
  ReadingSessionCache::save(kIsbn, 40, ReadingPhase::Paused);
  QVERIFY(!ReadingSessionCache::takeState(kIsbn).isEmpty());
  QVERIFY(ReadingSessionCache::takeState(kIsbn).isEmpty());
}

void ReadingSessionCacheTest::save_runningPhase_addsElapsedWallClock() {
  // Save as Running, then backdate lastSyncAt so takeState credits the gap.
  ReadingSessionCache::save(kIsbn, 100, ReadingPhase::Running);
  {
    QSettings settings;
    settings.beginGroup(u"readingSession/%1"_s.arg(kIsbn));
    settings.setValue("lastSyncAt", QDateTime::currentDateTimeUtc().addSecs(-30).toString(Qt::ISODate));
    settings.endGroup();
    settings.sync();
  }

  const QVariantMap out = ReadingSessionCache::takeState(kIsbn);
  QCOMPARE(out.value("phase").toInt(), static_cast<int>(ReadingPhase::Running));
  QVERIFY2(out.value("seconds").toInt() >= 130, "running timer must keep counting across the absence");
}

void ReadingSessionCacheTest::clear_removesEntry() {
  ReadingSessionCache::save(kIsbn, 10, ReadingPhase::Paused);
  ReadingSessionCache::clear(kIsbn);
  QVERIFY(ReadingSessionCache::takeState(kIsbn).isEmpty());
}

void ReadingSessionCacheTest::save_ignoresNonPositiveIsbn() {
  ReadingSessionCache::save(0, 10, ReadingPhase::Running);
  QVERIFY(ReadingSessionCache::takeState(0).isEmpty());
}

QTEST_GUILESS_MAIN(ReadingSessionCacheTest)
#include "ReadingSessionCacheTest.moc"
