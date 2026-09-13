#include "services/caching/ReadingProgressCache.hpp"

#include <QSettings>
#include <QStandardPaths>
#include <QTest>

using readary::services::ReadingProgressCache;

namespace {

constexpr qint64 kIsbn = 9780201616224LL;
constexpr qint64 kOtherIsbn = 9781491903995LL;

} // namespace

class ReadingProgressCacheTest : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void init();

  void has_withNothingSaved_isFalse();
  void save_thenHas_reportsIt();
  void takePagesRead_returnsWhatWasSaved();
  void takePagesRead_consumesTheEntry();
  void takePagesRead_withNothingSaved_isZero();
  void save_twice_keepsTheNewerValue();
  void save_ofZeroPages_isStillAnEntry();
  void clear_dropsTheEntry();
  void clear_withNothingSaved_isANoOp();
  void entries_areKeptPerBook();

  void save_withoutAnIsbn_isIgnored();
  void has_withoutAnIsbn_isFalse();
  void takePagesRead_withoutAnIsbn_isZero();
  void clear_withoutAnIsbn_isIgnored();
};

void ReadingProgressCacheTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  QCoreApplication::setOrganizationName("DarieszzBooksTests");
  QCoreApplication::setOrganizationDomain("tests.darieszzbooks.local");
  QCoreApplication::setApplicationName("ReadingProgressCacheTest");
}

void ReadingProgressCacheTest::init() {
  QSettings settings;
  settings.clear();
  settings.sync();
}

void ReadingProgressCacheTest::has_withNothingSaved_isFalse() { QVERIFY(!ReadingProgressCache::has(kIsbn)); }

void ReadingProgressCacheTest::save_thenHas_reportsIt() {
  ReadingProgressCache::save(kIsbn, 120);
  QVERIFY(ReadingProgressCache::has(kIsbn));
}

void ReadingProgressCacheTest::takePagesRead_returnsWhatWasSaved() {
  ReadingProgressCache::save(kIsbn, 120);
  QCOMPARE(ReadingProgressCache::takePagesRead(kIsbn), 120);
}

void ReadingProgressCacheTest::takePagesRead_consumesTheEntry() {
  // Restoring is a one-shot: the parked pages move back into the book.
  ReadingProgressCache::save(kIsbn, 120);

  QCOMPARE(ReadingProgressCache::takePagesRead(kIsbn), 120);

  QVERIFY(!ReadingProgressCache::has(kIsbn));
  QCOMPARE(ReadingProgressCache::takePagesRead(kIsbn), 0);
}

void ReadingProgressCacheTest::takePagesRead_withNothingSaved_isZero() {
  QCOMPARE(ReadingProgressCache::takePagesRead(kIsbn), 0);
}

void ReadingProgressCacheTest::save_twice_keepsTheNewerValue() {
  ReadingProgressCache::save(kIsbn, 120);
  ReadingProgressCache::save(kIsbn, 200);

  QCOMPARE(ReadingProgressCache::takePagesRead(kIsbn), 200);
}

void ReadingProgressCacheTest::save_ofZeroPages_isStillAnEntry() {
  // "Parked at page 0" is a real state, distinct from "nothing parked".
  ReadingProgressCache::save(kIsbn, 0);

  QVERIFY(ReadingProgressCache::has(kIsbn));
  QCOMPARE(ReadingProgressCache::takePagesRead(kIsbn), 0);
}

void ReadingProgressCacheTest::clear_dropsTheEntry() {
  ReadingProgressCache::save(kIsbn, 120);

  ReadingProgressCache::clear(kIsbn);

  QVERIFY(!ReadingProgressCache::has(kIsbn));
}

void ReadingProgressCacheTest::clear_withNothingSaved_isANoOp() {
  ReadingProgressCache::clear(kIsbn);
  QVERIFY(!ReadingProgressCache::has(kIsbn));
}

void ReadingProgressCacheTest::entries_areKeptPerBook() {
  ReadingProgressCache::save(kIsbn, 120);
  ReadingProgressCache::save(kOtherIsbn, 30);

  QCOMPARE(ReadingProgressCache::takePagesRead(kIsbn), 120);

  QVERIFY(ReadingProgressCache::has(kOtherIsbn));
  QCOMPARE(ReadingProgressCache::takePagesRead(kOtherIsbn), 30);
}

void ReadingProgressCacheTest::save_withoutAnIsbn_isIgnored() {
  ReadingProgressCache::save(0, 120);
  ReadingProgressCache::save(-1, 120);

  QVERIFY(!ReadingProgressCache::has(0));
  QVERIFY(!ReadingProgressCache::has(-1));
}

void ReadingProgressCacheTest::has_withoutAnIsbn_isFalse() { QVERIFY(!ReadingProgressCache::has(0)); }

void ReadingProgressCacheTest::takePagesRead_withoutAnIsbn_isZero() {
  QCOMPARE(ReadingProgressCache::takePagesRead(0), 0);
}

void ReadingProgressCacheTest::clear_withoutAnIsbn_isIgnored() { ReadingProgressCache::clear(0); }

QTEST_GUILESS_MAIN(ReadingProgressCacheTest)
#include "ReadingProgressCacheTest.moc"
