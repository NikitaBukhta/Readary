#include "controllers/ReadingStatisticsController.hpp"

#include "core/db/SqlQueryBuilder.hpp"
#include "services/dto/BookStatus.hpp"
#include "services/storage/BookTable.hpp"
#include "support/TempLibrary.hpp"

#include <QDate>
#include <QSignalSpy>
#include <QTest>
#include <QVariantList>
#include <QVariantMap>

#include <memory>

using Qt::StringLiterals::operator""_s;

using readary::controllers::ReadingStatisticsController;
using readary::core::SqlQueryBuilder;
using readary::services::BookDTO;
using readary::services::BookStatus;
using readary::tests::makeBook;
using readary::tests::TempLibrary;

namespace {

constexpr qint64 kFinishedIsbn = 9780201616224LL;
constexpr qint64 kInProgressIsbn = 9781491903995LL;

using Period = ReadingStatisticsController::Period;
using Granularity = ReadingStatisticsController::Granularity;

QDate today() { return QDate{2026, 3, 4}; }

BookDTO bookAt(qint64 isbn, const QString &name, int status, int pagesRead) {
  BookDTO book = makeBook(isbn, name, status);
  book.pagesRead = pagesRead;
  return book;
}

} // namespace

class ReadingStatisticsControllerTest : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void init();
  void cleanup();

  void newController_reportsNoData();
  void refresh_readsTheJournalOfEveryBook();
  void refresh_countsAFinishInTheCurrentMonth();
  void refresh_listsTheBooksReadAndOnTheGo();
  void refresh_staysQuietWhenNothingChanged();
  void refresh_picksUpASessionSavedAfterwards();
  void statistics_crossToQmlAsPlainObjects();
  void period_startsAtAllTime();
  void setPeriod_recomputesForThatPeriod();
  void setPeriod_doesNotReloadTheLibrary();
  void setPeriod_toTheSamePeriod_isANoOp();
  void setPeriod_outOfRange_isIgnored();
  void setPeriod_toCustomBeforeAnyRange_showsToday();
  void setCustomRange_switchesToCustom();
  void setCustomRange_thatDoesNotParse_changesNothing();
  void hasData_isAboutTheLibraryNotThePeriod();
  void hasData_changingAloneStillNotifies();

private:
  void addSession(qint64 isbn, const QString &startedAt, int minutes, int pagesFrom, int pagesTo);

  TempLibrary _library;
  std::unique_ptr<ReadingStatisticsController> _controller;
};

void ReadingStatisticsControllerTest::initTestCase() { Q_INIT_RESOURCE(db_scripts); }

void ReadingStatisticsControllerTest::init() {
  QVERIFY(_library.open());
  _controller = std::make_unique<ReadingStatisticsController>(_library.books(), &today, nullptr);
}

void ReadingStatisticsControllerTest::cleanup() {
  _controller.reset();
  _library.close();
}

void ReadingStatisticsControllerTest::addSession(qint64 isbn, const QString &startedAt, int minutes, int pagesFrom,
                                                 int pagesTo) {
  const QDateTime started = QDateTime::fromString(startedAt, Qt::ISODate);
  const QDateTime ended = started.addSecs(static_cast<qint64>(minutes) * 60);
  SqlQueryBuilder query;
  query
      .insertInto(u"reading_sessions"_s,
                  {u"book_isbn"_s, u"started_at"_s, u"ended_at"_s, u"pages_from"_s, u"pages_to"_s})
      .values({isbn, started.toString(Qt::ISODate), ended.toString(Qt::ISODate), pagesFrom, pagesTo});
  QString error;
  QVERIFY2(_library.db()->insert(query, &error) > 0, qPrintable(error));
}

void ReadingStatisticsControllerTest::newController_reportsNoData() {
  QCOMPARE(_controller->statistics().sessionCount, 0);
  QVERIFY(!_controller->hasData());
}

void ReadingStatisticsControllerTest::refresh_readsTheJournalOfEveryBook() {
  QCOMPARE(_library.books()->addBook(bookAt(kFinishedIsbn, u"Refactoring"_s, BookStatus::Finished, 300)),
           kFinishedIsbn);
  QCOMPARE(_library.books()->addBook(bookAt(kInProgressIsbn, u"Effective Modern C++"_s, BookStatus::InProgress, 30)),
           kInProgressIsbn);
  addSession(kFinishedIsbn, u"2026-03-02T10:00:00"_s, 30, 0, 30);
  addSession(kInProgressIsbn, u"2026-03-03T10:00:00"_s, 30, 0, 60);

  _controller->refresh();

  const auto &stats = _controller->statistics();
  QCOMPARE(stats.sessionCount, 2);
  QCOMPARE(stats.totalSeconds, 60 * 60);
  QCOMPARE(stats.minPagesPerHour, 60.0);
  QCOMPARE(stats.maxPagesPerHour, 120.0);
  QCOMPARE(stats.averagePagesPerHour, 90.0);
  QVERIFY(_controller->hasData());
}

void ReadingStatisticsControllerTest::refresh_countsAFinishInTheCurrentMonth() {
  QCOMPARE(_library.books()->addBook(bookAt(kFinishedIsbn, u"Refactoring"_s, BookStatus::Finished, 300)),
           kFinishedIsbn);
  addSession(kFinishedIsbn, u"2026-03-02T10:00:00"_s, 60, 0, 300);

  _controller->refresh();

  const auto &stats = _controller->statistics();
  QCOMPARE(stats.booksFinished, 1);
  QCOMPARE(stats.buckets.size(), 6);
  QCOMPARE(stats.buckets.constLast().books, 1);
}

void ReadingStatisticsControllerTest::refresh_listsTheBooksReadAndOnTheGo() {
  QCOMPARE(_library.books()->addBook(bookAt(kFinishedIsbn, u"Refactoring"_s, BookStatus::Finished, 300)),
           kFinishedIsbn);
  QCOMPARE(_library.books()->addBook(bookAt(kInProgressIsbn, u"Effective Modern C++"_s, BookStatus::InProgress, 120)),
           kInProgressIsbn);
  addSession(kFinishedIsbn, u"2026-03-04T10:00:00"_s, 30, 250, 300);

  _controller->refresh();

  const auto &booksRead = _controller->statistics().booksRead;
  QCOMPARE(booksRead.size(), 2);
  QCOMPARE(booksRead.at(0).isbn, kFinishedIsbn);
  QCOMPARE(booksRead.at(0).fromPage, 250);
  QCOMPARE(booksRead.at(0).toPage, 300);
  QCOMPARE(booksRead.at(1).isbn, kInProgressIsbn);
  QCOMPARE(booksRead.at(1).toPage, 120);
  QCOMPARE(booksRead.at(1).pagesInPeriod, 0);

  _controller->setPeriod(Period::Day);
  QCOMPARE(_controller->statistics().booksRead.size(), 1);
  QCOMPARE(_controller->statistics().booksRead.at(0).pagesInPeriod, 50);
}

void ReadingStatisticsControllerTest::refresh_staysQuietWhenNothingChanged() {
  QCOMPARE(_library.books()->addBook(bookAt(kInProgressIsbn, u"Effective Modern C++"_s, BookStatus::InProgress, 40)),
           kInProgressIsbn);
  addSession(kInProgressIsbn, u"2026-03-04T10:00:00"_s, 30, 0, 40);
  _controller->refresh();

  QSignalSpy spy{_controller.get(), &ReadingStatisticsController::statisticsChanged};
  _controller->refresh();

  QCOMPARE(spy.count(), 0);
}

void ReadingStatisticsControllerTest::refresh_picksUpASessionSavedAfterwards() {
  QCOMPARE(_library.books()->addBook(bookAt(kInProgressIsbn, u"Effective Modern C++"_s, BookStatus::InProgress, 40)),
           kInProgressIsbn);
  _controller->refresh();
  QSignalSpy spy{_controller.get(), &ReadingStatisticsController::statisticsChanged};

  addSession(kInProgressIsbn, u"2026-03-04T10:00:00"_s, 30, 0, 40);
  _controller->refresh();

  QCOMPARE(spy.count(), 1);
  QCOMPARE(_controller->statistics().sessionCount, 1);
  QCOMPARE(_controller->statistics().totalSeconds, 30 * 60);
}

void ReadingStatisticsControllerTest::statistics_crossToQmlAsPlainObjects() {
  QCOMPARE(_library.books()->addBook(bookAt(kInProgressIsbn, u"Effective Modern C++"_s, BookStatus::InProgress, 120)),
           kInProgressIsbn);
  _controller->refresh();

  const auto &stats = _controller->statistics();
  const QVariantList buckets = stats.bucketsAsVariantList();
  QCOMPARE(buckets.size(), 6);
  const QVariantMap last = buckets.constLast().toMap();
  for (const QString &key : {u"year"_s, u"month"_s, u"day"_s, u"hour"_s, u"pages"_s, u"books"_s}) {
    QVERIFY2(last.contains(key), qPrintable(key));
  }
  QCOMPARE(last.value(u"year"_s).toInt(), 2026);
  QCOMPARE(last.value(u"month"_s).toInt(), 3);
  QCOMPARE(stats.rangeEnd(), u"2026-03-31"_s);

  const QVariantList books = stats.booksReadAsVariantList();
  QCOMPARE(books.size(), 1);
  const QVariantMap row = books.constFirst().toMap();
  QCOMPARE(row.value(u"isbn"_s).toLongLong(), kInProgressIsbn);
  QCOMPARE(row.value(u"name"_s).toString(), u"Effective Modern C++"_s);
  QCOMPARE(row.value(u"fromPage"_s).toInt(), 120);
  QCOMPARE(row.value(u"toPage"_s).toInt(), 120);
  QCOMPARE(row.value(u"pagesInPeriod"_s).toInt(), 0);
  QCOMPARE(row.value(u"totalPages"_s).toInt(), 300);
}

void ReadingStatisticsControllerTest::period_startsAtAllTime() { QCOMPARE(_controller->period(), Period::AllTime); }

void ReadingStatisticsControllerTest::setPeriod_recomputesForThatPeriod() {
  QCOMPARE(_library.books()->addBook(bookAt(kInProgressIsbn, u"Effective Modern C++"_s, BookStatus::InProgress, 40)),
           kInProgressIsbn);
  addSession(kInProgressIsbn, u"2026-03-04T00:10:00"_s, 30, 0, 40);
  addSession(kInProgressIsbn, u"2026-03-03T23:50:00"_s, 30, 0, 99);
  _controller->refresh();
  QSignalSpy periodSpy{_controller.get(), &ReadingStatisticsController::periodChanged};
  QSignalSpy statisticsSpy{_controller.get(), &ReadingStatisticsController::statisticsChanged};

  _controller->setPeriod(Period::Day);

  QCOMPARE(periodSpy.count(), 1);
  QCOMPARE(statisticsSpy.count(), 1);
  const auto &stats = _controller->statistics();
  QCOMPARE(stats.granularity(), static_cast<int>(Granularity::ByHour));
  QCOMPARE(stats.buckets.size(), 24);
  QCOMPARE(stats.rangeStart(), u"2026-03-04"_s);
  QCOMPARE(stats.sessionCount, 1);
  QCOMPARE(stats.pagesRead, 40);
}

void ReadingStatisticsControllerTest::setPeriod_doesNotReloadTheLibrary() {
  _controller->refresh();
  QCOMPARE(_library.books()->addBook(bookAt(kInProgressIsbn, u"Effective Modern C++"_s, BookStatus::InProgress, 40)),
           kInProgressIsbn);
  addSession(kInProgressIsbn, u"2026-03-04T10:00:00"_s, 30, 0, 40);

  _controller->setPeriod(Period::Day);
  QCOMPARE(_controller->statistics().sessionCount, 0);

  _controller->refresh();
  QCOMPARE(_controller->statistics().sessionCount, 1);
}

void ReadingStatisticsControllerTest::setPeriod_toTheSamePeriod_isANoOp() {
  _controller->setPeriod(Period::Week);
  QSignalSpy periodSpy{_controller.get(), &ReadingStatisticsController::periodChanged};

  _controller->setPeriod(Period::Week);

  QCOMPARE(periodSpy.count(), 0);
}

void ReadingStatisticsControllerTest::setPeriod_outOfRange_isIgnored() {
  QSignalSpy periodSpy{_controller.get(), &ReadingStatisticsController::periodChanged};

  _controller->setProperty("period", 42);

  QCOMPARE(periodSpy.count(), 0);
  QCOMPARE(_controller->period(), Period::AllTime);
}

void ReadingStatisticsControllerTest::setPeriod_toCustomBeforeAnyRange_showsToday() {
  _controller->setPeriod(Period::Custom);

  QCOMPARE(_controller->period(), Period::Custom);
  QCOMPARE(_controller->statistics().rangeStart(), u"2026-03-04"_s);
  QCOMPARE(_controller->statistics().rangeEnd(), u"2026-03-04"_s);
}

void ReadingStatisticsControllerTest::setCustomRange_switchesToCustom() {
  QSignalSpy periodSpy{_controller.get(), &ReadingStatisticsController::periodChanged};

  QVERIFY(_controller->setCustomRange(u"2026-02-20"_s, u"2026-02-10"_s));

  QCOMPARE(periodSpy.count(), 1);
  QCOMPARE(_controller->period(), Period::Custom);
  const auto &stats = _controller->statistics();
  QCOMPARE(stats.rangeStart(), u"2026-02-10"_s);
  QCOMPARE(stats.rangeEnd(), u"2026-02-20"_s);
  QCOMPARE(stats.granularity(), static_cast<int>(Granularity::ByDay));
  QCOMPARE(stats.buckets.size(), 11);

  QVERIFY(_controller->setCustomRange(u"2026-01-01"_s, u"2026-06-30"_s));
  QCOMPARE(periodSpy.count(), 1);
  QCOMPARE(_controller->statistics().granularity(), static_cast<int>(Granularity::ByMonth));
}

void ReadingStatisticsControllerTest::setCustomRange_thatDoesNotParse_changesNothing() {
  QSignalSpy periodSpy{_controller.get(), &ReadingStatisticsController::periodChanged};

  QVERIFY(!_controller->setCustomRange(u"yesterday"_s, u"2026-02-10"_s));
  QVERIFY(!_controller->setCustomRange(u"2026-02-10"_s, u""_s));

  QCOMPARE(periodSpy.count(), 0);
  QCOMPARE(_controller->period(), Period::AllTime);
}

void ReadingStatisticsControllerTest::hasData_isAboutTheLibraryNotThePeriod() {
  QCOMPARE(_library.books()->addBook(bookAt(kInProgressIsbn, u"Effective Modern C++"_s, BookStatus::InProgress, 40)),
           kInProgressIsbn);
  addSession(kInProgressIsbn, u"2026-03-04T10:00:00"_s, 30, 0, 40);

  QVERIFY(_controller->setCustomRange(u"2001-01-01"_s, u"2001-01-31"_s));

  QCOMPARE(_controller->statistics().sessionCount, 0);
  QVERIFY(_controller->hasData());
}

void ReadingStatisticsControllerTest::hasData_changingAloneStillNotifies() {
  _controller->setPeriod(Period::Day);
  QVERIFY(!_controller->hasData());
  QSignalSpy spy{_controller.get(), &ReadingStatisticsController::statisticsChanged};

  QCOMPARE(_library.books()->addBook(bookAt(kInProgressIsbn, u"Effective Modern C++"_s, BookStatus::InProgress, 40)),
           kInProgressIsbn);
  _controller->refresh();

  QVERIFY(_controller->statistics().booksRead.isEmpty());
  QVERIFY(_controller->hasData());
  QCOMPARE(spy.count(), 1);
}

QTEST_GUILESS_MAIN(ReadingStatisticsControllerTest)
#include "ReadingStatisticsControllerTest.moc"
