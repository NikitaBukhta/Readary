#include "controllers/ReadingStatisticsController.hpp"

#include "services/dto/BookStatus.hpp"
#include "services/storage/BookTable.hpp"
#include "support/TempLibrary.hpp"

#include <QSignalSpy>
#include <QTest>
#include <QVariantMap>

#include <memory>

using Qt::StringLiterals::operator""_s;

using readary::controllers::ReadingStatisticsController;
using readary::services::BookDTO;
using readary::services::BookStatus;
using readary::tests::makeBook;
using readary::tests::TempLibrary;

namespace {

constexpr qint64 kFinishedIsbn = 9780201616224LL;
constexpr qint64 kInProgressIsbn = 9781491903995LL;

// makeBook gives every book 300 pages; the position is what varies here.
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
  void refresh_listsTheBooksInProgress();
  void refresh_staysQuietWhenNothingChanged();
  void refresh_picksUpASessionSavedAfterwards();
  void statistics_crossToQmlAsPlainObjects();

private:
  TempLibrary _library;
  std::unique_ptr<ReadingStatisticsController> _controller;
};

void ReadingStatisticsControllerTest::initTestCase() {
  // Resources of a static lib can be stripped by the linker — see main.cpp.
  Q_INIT_RESOURCE(db_scripts);
}

void ReadingStatisticsControllerTest::init() {
  QVERIFY(_library.open());
  _controller = std::make_unique<ReadingStatisticsController>(_library.books(), nullptr);
}

void ReadingStatisticsControllerTest::cleanup() {
  _controller.reset();
  _library.close();
}

void ReadingStatisticsControllerTest::newController_reportsNoData() {
  // Nothing is read in the constructor — the page asks on open.
  QCOMPARE(_controller->statistics().sessionCount, 0);
  QVERIFY(!_controller->hasData());
}

void ReadingStatisticsControllerTest::refresh_readsTheJournalOfEveryBook() {
  QCOMPARE(_library.books()->addBook(bookAt(kFinishedIsbn, u"Refactoring"_s, BookStatus::Finished, 300)),
           kFinishedIsbn);
  QCOMPARE(_library.books()->addBook(bookAt(kInProgressIsbn, u"Effective Modern C++"_s, BookStatus::InProgress, 30)),
           kInProgressIsbn);
  // 60 p/h and 120 p/h.
  QVERIFY(_library.books()->insertReadingSession(kFinishedIsbn, 0, 30, 30 * 60) > 0);
  QVERIFY(_library.books()->insertReadingSession(kInProgressIsbn, 0, 60, 30 * 60) > 0);

  _controller->refresh();

  const auto stats = _controller->statistics();
  QCOMPARE(stats.sessionCount, 2);
  QCOMPARE(stats.totalSeconds, 60 * 60);
  QCOMPARE(stats.minPagesPerHour, 60.0);
  QCOMPARE(stats.maxPagesPerHour, 120.0);
  QCOMPARE(stats.averagePagesPerHour, 90.0);
  QVERIFY(_controller->hasData());
}

void ReadingStatisticsControllerTest::refresh_countsAFinishInTheCurrentMonth() {
  // insertReadingSession stamps the row as ending now, so the book lands in
  // the newest month of the series.
  QCOMPARE(_library.books()->addBook(bookAt(kFinishedIsbn, u"Refactoring"_s, BookStatus::Finished, 300)),
           kFinishedIsbn);
  QVERIFY(_library.books()->insertReadingSession(kFinishedIsbn, 0, 300, 60 * 60) > 0);

  _controller->refresh();

  const auto stats = _controller->statistics();
  QCOMPARE(stats.booksFinished, 1);
  QCOMPARE(stats.monthlyBooks.size(), readary::services::ReadingStatisticsDTO::kMonthsShown);
  QCOMPARE(stats.monthlyBooks.constLast().books, 1);
}

void ReadingStatisticsControllerTest::refresh_listsTheBooksInProgress() {
  QCOMPARE(_library.books()->addBook(bookAt(kFinishedIsbn, u"Refactoring"_s, BookStatus::Finished, 300)),
           kFinishedIsbn);
  QCOMPARE(_library.books()->addBook(bookAt(kInProgressIsbn, u"Effective Modern C++"_s, BookStatus::InProgress, 120)),
           kInProgressIsbn);

  _controller->refresh();

  const auto &inProgress = _controller->statistics().booksInProgress;
  QCOMPARE(inProgress.size(), 1);
  QCOMPARE(inProgress.at(0).isbn, kInProgressIsbn);
  QCOMPARE(inProgress.at(0).pagesRead, 120);
  QCOMPARE(inProgress.at(0).totalPages, 300);
  // Never timed, but a book on the go is still something to show.
  QVERIFY(_controller->hasData());
}

void ReadingStatisticsControllerTest::refresh_staysQuietWhenNothingChanged() {
  QCOMPARE(_library.books()->addBook(bookAt(kInProgressIsbn, u"Effective Modern C++"_s, BookStatus::InProgress, 40)),
           kInProgressIsbn);
  QVERIFY(_library.books()->insertReadingSession(kInProgressIsbn, 0, 40, 30 * 60) > 0);
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

  QVERIFY(_library.books()->insertReadingSession(kInProgressIsbn, 0, 40, 30 * 60) > 0);
  _controller->refresh();

  QCOMPARE(spy.count(), 1);
  QCOMPARE(_controller->statistics().sessionCount, 1);
  QCOMPARE(_controller->statistics().totalSeconds, 30 * 60);
}

void ReadingStatisticsControllerTest::statistics_crossToQmlAsPlainObjects() {
  QCOMPARE(_library.books()->addBook(bookAt(kInProgressIsbn, u"Effective Modern C++"_s, BookStatus::InProgress, 120)),
           kInProgressIsbn);
  _controller->refresh();

  const auto stats = _controller->statistics();
  const QVariantList months = stats.monthlyBooksAsVariantList();
  QCOMPARE(months.size(), readary::services::ReadingStatisticsDTO::kMonthsShown);
  QVERIFY(months.constLast().toMap().contains(u"books"_s));
  QVERIFY(months.constLast().toMap().contains(u"month"_s));

  const QVariantList books = stats.booksInProgressAsVariantList();
  QCOMPARE(books.size(), 1);
  const QVariantMap row = books.constFirst().toMap();
  QCOMPARE(row.value(u"isbn"_s).toLongLong(), kInProgressIsbn);
  QCOMPARE(row.value(u"name"_s).toString(), u"Effective Modern C++"_s);
  QCOMPARE(row.value(u"pagesRead"_s).toInt(), 120);
  QCOMPARE(row.value(u"totalPages"_s).toInt(), 300);
}

QTEST_GUILESS_MAIN(ReadingStatisticsControllerTest)
#include "ReadingStatisticsControllerTest.moc"
