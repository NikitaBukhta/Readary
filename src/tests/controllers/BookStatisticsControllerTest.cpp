#include "controllers/BookStatisticsController.hpp"
#include "services/storage/BookTable.hpp"
#include "support/TempLibrary.hpp"

#include <QSignalSpy>
#include <QTest>

#include <memory>

using Qt::StringLiterals::operator""_s;

using readary::controllers::BookStatisticsController;
using readary::tests::makeBook;
using readary::tests::TempLibrary;

namespace {

constexpr qint64 kIsbn = 9780201616224LL;
constexpr qint64 kOtherIsbn = 9781491903995LL;

} // namespace

class BookStatisticsControllerTest : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void init();
  void cleanup();

  void selectingABook_computesItsStatistics();
  void statisticsCoverOnlyTheSelectedBook();
  void switchingBook_recomputesAndNotifies();
  void selectingTheSameBookAgain_isANoOp();
  void refresh_staysQuietWhenNothingChanged();
  void unknownBook_reportsNoData();
  void clearingTheBook_dropsTheStatistics();
  void refresh_picksUpASessionSavedAfterwards();
  void weeklyPages_bucketTheJournalReadBackFromTheDatabase();

private:
  TempLibrary _library;
  std::unique_ptr<BookStatisticsController> _controller;
};

void BookStatisticsControllerTest::initTestCase() {
  // Resources of a static lib can be stripped by the linker — see main.cpp.
  Q_INIT_RESOURCE(db_scripts);
}

void BookStatisticsControllerTest::init() {
  QVERIFY(_library.open());
  QCOMPARE(_library.books()->addBook(makeBook(kIsbn, u"Refactoring"_s)), kIsbn);
  QCOMPARE(_library.books()->addBook(makeBook(kOtherIsbn, u"Effective Modern C++"_s)), kOtherIsbn);

  // 30 pages in 30 minutes, then 60 pages in 30 minutes.
  QVERIFY(_library.books()->insertReadingSession(kIsbn, 0, 30, 30 * 60) > 0);
  QVERIFY(_library.books()->insertReadingSession(kIsbn, 30, 90, 30 * 60) > 0);
  QVERIFY(_library.books()->insertReadingSession(kOtherIsbn, 0, 10, 60 * 60) > 0);

  _controller = std::make_unique<BookStatisticsController>(_library.books(), nullptr);
}

void BookStatisticsControllerTest::cleanup() {
  _controller.reset();
  _library.close();
}

void BookStatisticsControllerTest::selectingABook_computesItsStatistics() {
  _controller->setBookIsbn(kIsbn);

  const auto stats = _controller->statistics();
  QCOMPARE(stats.sessionCount, 2);
  QCOMPARE(stats.pagesRead, 90);
  QCOMPARE(stats.totalSeconds, 60 * 60);
  QCOMPARE(stats.averagePagesPerHour, 90.0);
  QCOMPARE(stats.minPagesPerHour, 60.0);
  QCOMPARE(stats.maxPagesPerHour, 120.0);
  QCOMPARE(stats.progressPoints.size(), 2);
  QVERIFY(_controller->hasData());
}

void BookStatisticsControllerTest::statisticsCoverOnlyTheSelectedBook() {
  _controller->setBookIsbn(kOtherIsbn);

  const auto stats = _controller->statistics();
  QCOMPARE(stats.sessionCount, 1);
  QCOMPARE(stats.pagesRead, 10);
}

void BookStatisticsControllerTest::switchingBook_recomputesAndNotifies() {
  _controller->setBookIsbn(kIsbn);
  QSignalSpy spy{_controller.get(), &BookStatisticsController::statisticsChanged};

  _controller->setBookIsbn(kOtherIsbn);

  QCOMPARE(spy.count(), 1);
  QCOMPARE(_controller->bookIsbn(), kOtherIsbn);
  QCOMPARE(_controller->statistics().pagesRead, 10);
}

void BookStatisticsControllerTest::selectingTheSameBookAgain_isANoOp() {
  // BookController re-emits currentBookIsbnChanged on every book-list reset,
  // so this setter is called far more often than the open book changes. Only
  // a journal write should cost a re-read, and that arrives on its own signal.
  _controller->setBookIsbn(kIsbn);
  QVERIFY(_library.books()->insertReadingSession(kIsbn, 90, 120, 30 * 60) > 0);

  QSignalSpy spy{_controller.get(), &BookStatisticsController::statisticsChanged};
  _controller->setBookIsbn(kIsbn);

  QCOMPARE(spy.count(), 0);
  QCOMPARE(_controller->statistics().sessionCount, 2);
}

void BookStatisticsControllerTest::refresh_staysQuietWhenNothingChanged() {
  // The page refreshes on every open. Re-emitting for an identical result
  // would tear down and rebuild every chart delegate for nothing.
  _controller->setBookIsbn(kIsbn);
  QSignalSpy spy{_controller.get(), &BookStatisticsController::statisticsChanged};

  _controller->refresh();
  _controller->refresh();

  QCOMPARE(spy.count(), 0);
}

void BookStatisticsControllerTest::unknownBook_reportsNoData() {
  _controller->setBookIsbn(9780000000000LL);

  QVERIFY(!_controller->hasData());
  QCOMPARE(_controller->statistics().sessionCount, 0);
  QCOMPARE(_controller->statistics().weeklyPages.size(), 7);
}

void BookStatisticsControllerTest::clearingTheBook_dropsTheStatistics() {
  _controller->setBookIsbn(kIsbn);
  QVERIFY(_controller->hasData());

  _controller->setBookIsbn(0);

  QVERIFY(!_controller->hasData());
  QCOMPARE(_controller->statistics().pagesRead, 0);
  QVERIFY(_controller->statistics().progressPoints.isEmpty());
}

void BookStatisticsControllerTest::refresh_picksUpASessionSavedAfterwards() {
  _controller->setBookIsbn(kIsbn);
  QCOMPARE(_controller->statistics().sessionCount, 2);

  QVERIFY(_library.books()->insertReadingSession(kIsbn, 90, 150, 60 * 60) > 0);
  QSignalSpy spy{_controller.get(), &BookStatisticsController::statisticsChanged};

  _controller->refresh();

  QCOMPARE(spy.count(), 1);
  QCOMPARE(_controller->statistics().sessionCount, 3);
  QCOMPARE(_controller->statistics().pagesRead, 150);
}

void BookStatisticsControllerTest::weeklyPages_bucketTheJournalReadBackFromTheDatabase() {
  // The point is that the buckets survive the whole real path — BookTable
  // writes UTC, ReadingSessionDTO reads local, the calculator buckets on the
  // local date. Which weekday "now" lands on is deliberately not asserted:
  // the fixture back-dates each session by its duration, so a run just after
  // midnight would otherwise fail, and a run just after midnight on a Monday
  // would put the whole fixture in the previous week. The conversion itself
  // is pinned by ReadingSessionDTOTest::fromMap_normalizesStampsToLocalTime.
  _controller->setBookIsbn(kIsbn);

  const auto weekly = _controller->statistics().weeklyPages;
  QCOMPARE(weekly.size(), 7);

  int total = 0;
  int daysWithReading = 0;
  for (const int pages : weekly) {
    total += pages;
    if (pages > 0) {
      ++daysWithReading;
    }
  }
  QCOMPARE(total, 90);
  QCOMPARE(daysWithReading, 1);
}

QTEST_GUILESS_MAIN(BookStatisticsControllerTest)
#include "BookStatisticsControllerTest.moc"
