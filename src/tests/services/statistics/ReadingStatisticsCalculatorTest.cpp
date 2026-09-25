#include "services/statistics/ReadingStatisticsCalculator.hpp"

#include "services/dto/BookStatus.hpp"
#include "services/statistics/StatisticsPeriods.hpp"

#include <QDate>
#include <QList>
#include <QTest>

using Qt::StringLiterals::operator""_s;

using readary::services::BookDTO;
using readary::services::BookStatus;
using readary::services::PeriodBucketDTO;
using readary::services::ReadingSessionDTO;
using readary::services::ReadingStatisticsCalculator;
using readary::services::ReadingStatisticsDTO;
using readary::services::StatisticsPeriods;
using readary::services::StatisticsRange;

namespace {

constexpr qint64 kFirstIsbn = 9780201616224LL;
constexpr qint64 kSecondIsbn = 9781491903995LL;
constexpr qint64 kThirdIsbn = 9780134685991LL;

BookDTO makeBook(qint64 isbn, const QString &name, int status, int pagesRead = 0, int totalPages = 300) {
  BookDTO book;
  book.isbn = isbn;
  book.name = name;
  book.status = status;
  book.pagesRead = pagesRead;
  book.totalPages = totalPages;
  return book;
}

ReadingSessionDTO makeSession(qint64 bookIsbn, const QString &startedAt, int minutes, int pagesFrom, int pagesTo) {
  ReadingSessionDTO session;
  session.bookIsbn = bookIsbn;
  session.startedAt = QDateTime::fromString(startedAt, Qt::ISODate);
  session.endedAt = session.startedAt.addSecs(static_cast<qint64>(minutes) * 60);
  session.pagesFrom = pagesFrom;
  session.pagesTo = pagesTo;
  return session;
}

QDate today() { return QDate{2026, 3, 4}; }

StatisticsRange allTime(const QList<ReadingSessionDTO> &sessions = {}) {
  return StatisticsPeriods::allTime(today(), ReadingStatisticsCalculator::earliestSession(sessions));
}

QList<int> pagesOf(const ReadingStatisticsDTO &stats) {
  QList<int> pages;
  for (const PeriodBucketDTO &bucket : stats.buckets) {
    pages.append(bucket.pages);
  }
  return pages;
}

QList<int> booksOf(const ReadingStatisticsDTO &stats) {
  QList<int> books;
  for (const PeriodBucketDTO &bucket : stats.buckets) {
    books.append(bucket.books);
  }
  return books;
}

QList<ReadingSessionDTO> weekOfSessions() {
  return {
      makeSession(kFirstIsbn, u"2026-03-02T10:00:00"_s, 60, 0, 20),
      makeSession(kSecondIsbn, u"2026-03-02T20:00:00"_s, 30, 10, 40),
      makeSession(kSecondIsbn, u"2026-03-08T09:00:00"_s, 30, 40, 55),
      makeSession(kFirstIsbn, u"2026-02-27T10:00:00"_s, 30, 0, 99),
  };
}

} // namespace

class ReadingStatisticsCalculatorTest : public QObject {
  Q_OBJECT

private slots:
  void emptyLibrary_keepsTheBucketsButCountsNothing();
  void week_takesOnlyTheSessionsThatStartedInIt();
  void week_bucketsPagesByDay();
  void day_bucketsPagesByHour();
  void year_bucketsByMonth();
  void allTime_takesTheWholeJournal();
  void booksFinished_areDatedByTheirLastSession();
  void booksFinished_undatedOnlyCountForAllTime();
  void sessionAcrossMidnight_staysInThePeriodItStarted();
  void booksRead_showWhatThePeriodMoved();
  void booksRead_leaveOutBooksNotReadInThePeriod();
  void booksRead_allTimeAddsUntimedBooksInProgress();
  void booksRead_missingEndPageHoldsPosition();
  void booksRead_areCappedMostRecentFirst();
  void earliestSession_isTheFirstStart();
  void twoLibrariesWithTheSameFigures_compareEqual();
};

void ReadingStatisticsCalculatorTest::emptyLibrary_keepsTheBucketsButCountsNothing() {
  const ReadingStatisticsDTO stats = ReadingStatisticsCalculator::compute({}, {}, StatisticsPeriods::week(today()));

  QCOMPARE(stats.booksFinished, 0);
  QCOMPARE(stats.sessionCount, 0);
  QCOMPARE(stats.totalSeconds, 0);
  QCOMPARE(stats.buckets.size(), 7);
  QCOMPARE(pagesOf(stats), QList<int>(7, 0));
  QVERIFY(stats.booksRead.isEmpty());
}

void ReadingStatisticsCalculatorTest::week_takesOnlyTheSessionsThatStartedInIt() {
  const ReadingStatisticsDTO stats =
      ReadingStatisticsCalculator::compute({}, weekOfSessions(), StatisticsPeriods::week(today()));

  QCOMPARE(stats.sessionCount, 3);
  QCOMPARE(stats.pagesRead, 20 + 30 + 15);
  QCOMPARE(stats.totalSeconds, 120 * 60);
  QCOMPARE(stats.minPagesPerHour, 20.0);
  QCOMPARE(stats.maxPagesPerHour, 60.0);
}

void ReadingStatisticsCalculatorTest::week_bucketsPagesByDay() {
  const ReadingStatisticsDTO stats =
      ReadingStatisticsCalculator::compute({}, weekOfSessions(), StatisticsPeriods::week(today()));

  QCOMPARE(pagesOf(stats), (QList<int>{50, 0, 0, 0, 0, 0, 15}));
  QCOMPARE(stats.buckets.constFirst().date, QDate(2026, 3, 2));
  QCOMPARE(stats.buckets.constLast().date, QDate(2026, 3, 8));
}

void ReadingStatisticsCalculatorTest::day_bucketsPagesByHour() {
  const QList<ReadingSessionDTO> sessions{
      makeSession(kFirstIsbn, u"2026-03-04T07:15:00"_s, 30, 0, 12),
      makeSession(kFirstIsbn, u"2026-03-04T21:40:00"_s, 30, 12, 30),
      makeSession(kFirstIsbn, u"2026-03-03T21:40:00"_s, 30, 0, 99),
  };

  const ReadingStatisticsDTO stats =
      ReadingStatisticsCalculator::compute({}, sessions, StatisticsPeriods::day(today()));

  QCOMPARE(stats.buckets.size(), 24);
  QCOMPARE(stats.buckets.at(7).pages, 12);
  QCOMPARE(stats.buckets.at(7).hour, 7);
  QCOMPARE(stats.buckets.at(21).pages, 18);
  QCOMPARE(stats.pagesRead, 30);
}

void ReadingStatisticsCalculatorTest::year_bucketsByMonth() {
  const QList<ReadingSessionDTO> sessions{
      makeSession(kFirstIsbn, u"2026-01-20T10:00:00"_s, 60, 0, 40),
      makeSession(kFirstIsbn, u"2026-03-01T10:00:00"_s, 60, 40, 50),
      makeSession(kFirstIsbn, u"2025-12-31T10:00:00"_s, 60, 0, 99),
  };

  const ReadingStatisticsDTO stats =
      ReadingStatisticsCalculator::compute({}, sessions, StatisticsPeriods::year(today()));

  QCOMPARE(stats.buckets.size(), 12);
  QCOMPARE(stats.buckets.at(0).pages, 40);
  QCOMPARE(stats.buckets.at(2).pages, 10);
  QCOMPARE(stats.buckets.at(2).date, QDate(2026, 3, 1));
  QCOMPARE(stats.sessionCount, 2);
}

void ReadingStatisticsCalculatorTest::allTime_takesTheWholeJournal() {
  const QList<ReadingSessionDTO> sessions = weekOfSessions();

  const ReadingStatisticsDTO stats = ReadingStatisticsCalculator::compute({}, sessions, allTime(sessions));

  QCOMPARE(stats.sessionCount, 4);
  QCOMPARE(stats.buckets.size(), 6);
  QCOMPARE(pagesOf(stats), (QList<int>{0, 0, 0, 0, 99, 65}));
}

void ReadingStatisticsCalculatorTest::booksFinished_areDatedByTheirLastSession() {
  const QList<BookDTO> books{
      makeBook(kFirstIsbn, u"Refactoring"_s, BookStatus::Finished),
      makeBook(kSecondIsbn, u"Effective Modern C++"_s, BookStatus::Finished),
      makeBook(kThirdIsbn, u"Clean Architecture"_s, BookStatus::InProgress),
  };
  const QList<ReadingSessionDTO> sessions{
      makeSession(kFirstIsbn, u"2026-02-26T10:00:00"_s, 60, 0, 150),
      makeSession(kFirstIsbn, u"2026-03-03T10:00:00"_s, 60, 150, 300),
      makeSession(kSecondIsbn, u"2026-02-25T10:00:00"_s, 60, 0, 300),
      makeSession(kThirdIsbn, u"2026-03-04T10:00:00"_s, 60, 0, 50),
  };

  const ReadingStatisticsDTO stats =
      ReadingStatisticsCalculator::compute(books, sessions, StatisticsPeriods::week(today()));

  QCOMPARE(stats.booksFinished, 1);
  QCOMPARE(booksOf(stats), (QList<int>{0, 1, 0, 0, 0, 0, 0}));
}

void ReadingStatisticsCalculatorTest::booksFinished_undatedOnlyCountForAllTime() {
  const QList<BookDTO> books{makeBook(kFirstIsbn, u"Refactoring"_s, BookStatus::Finished, 300)};

  QCOMPARE(ReadingStatisticsCalculator::compute(books, {}, StatisticsPeriods::week(today())).booksFinished, 0);
  QCOMPARE(ReadingStatisticsCalculator::compute(books, {}, StatisticsPeriods::year(today())).booksFinished, 0);

  const ReadingStatisticsDTO everything = ReadingStatisticsCalculator::compute(books, {}, allTime());
  QCOMPARE(everything.booksFinished, 1);
  QCOMPARE(booksOf(everything), QList<int>(6, 0));
}

void ReadingStatisticsCalculatorTest::sessionAcrossMidnight_staysInThePeriodItStarted() {
  const QList<BookDTO> books{makeBook(kFirstIsbn, u"Refactoring"_s, BookStatus::Finished, 300)};
  const QList<ReadingSessionDTO> sessions{makeSession(kFirstIsbn, u"2026-03-01T23:40:00"_s, 40, 250, 300)};

  const ReadingStatisticsDTO lastWeek =
      ReadingStatisticsCalculator::compute(books, sessions, StatisticsPeriods::week(QDate{2026, 2, 25}));
  QCOMPARE(lastWeek.pagesRead, 50);
  QCOMPARE(lastWeek.booksFinished, 1);
  QCOMPARE(lastWeek.booksRead.size(), 1);

  const ReadingStatisticsDTO thisWeek =
      ReadingStatisticsCalculator::compute(books, sessions, StatisticsPeriods::week(today()));
  QCOMPARE(thisWeek.pagesRead, 0);
  QCOMPARE(thisWeek.booksFinished, 0);
  QVERIFY(thisWeek.booksRead.isEmpty());
}

void ReadingStatisticsCalculatorTest::booksRead_showWhatThePeriodMoved() {
  const QList<BookDTO> books{
      makeBook(kFirstIsbn, u"Refactoring"_s, BookStatus::InProgress, 99, 300),
      makeBook(kSecondIsbn, u"Effective Modern C++"_s, BookStatus::InProgress, 55, 400),
  };

  const ReadingStatisticsDTO stats =
      ReadingStatisticsCalculator::compute(books, weekOfSessions(), StatisticsPeriods::week(today()));

  QCOMPARE(stats.booksRead.size(), 2);
  const auto &second = stats.booksRead.at(0);
  QCOMPARE(second.isbn, kSecondIsbn);
  QCOMPARE(second.fromPage, 10);
  QCOMPARE(second.toPage, 55);
  QCOMPARE(second.pagesInPeriod, 45);
  QCOMPARE(second.totalPages, 400);
  const auto &first = stats.booksRead.at(1);
  QCOMPARE(first.isbn, kFirstIsbn);
  QCOMPARE(first.fromPage, 0);
  QCOMPARE(first.toPage, 20);
  QCOMPARE(first.pagesInPeriod, 20);
}

void ReadingStatisticsCalculatorTest::booksRead_leaveOutBooksNotReadInThePeriod() {
  const QList<BookDTO> books{
      makeBook(kFirstIsbn, u"Refactoring"_s, BookStatus::InProgress, 99),
      makeBook(kThirdIsbn, u"Clean Architecture"_s, BookStatus::InProgress, 40),
  };
  const QList<ReadingSessionDTO> sessions{makeSession(kFirstIsbn, u"2026-03-03T10:00:00"_s, 60, 0, 99)};

  QVERIFY(ReadingStatisticsCalculator::compute(books, sessions, StatisticsPeriods::day(today())).booksRead.isEmpty());
  QCOMPARE(ReadingStatisticsCalculator::compute(books, sessions, StatisticsPeriods::week(today())).booksRead.size(), 1);
}

void ReadingStatisticsCalculatorTest::booksRead_allTimeAddsUntimedBooksInProgress() {
  const QList<BookDTO> books{
      makeBook(kFirstIsbn, u"Refactoring"_s, BookStatus::InProgress, 20),
      makeBook(kThirdIsbn, u"Alpha"_s, BookStatus::InProgress, 40, 0),
      makeBook(kSecondIsbn, u"Effective Modern C++"_s, BookStatus::Finished, 300),
  };
  const QList<ReadingSessionDTO> sessions{makeSession(kFirstIsbn, u"2026-03-03T10:00:00"_s, 60, 0, 20)};

  const ReadingStatisticsDTO stats = ReadingStatisticsCalculator::compute(books, sessions, allTime(sessions));

  QCOMPARE(stats.booksRead.size(), 2);
  QCOMPARE(stats.booksRead.at(0).isbn, kFirstIsbn);
  const auto &untimed = stats.booksRead.at(1);
  QCOMPARE(untimed.isbn, kThirdIsbn);
  QCOMPARE(untimed.fromPage, 40);
  QCOMPARE(untimed.toPage, 40);
  QCOMPARE(untimed.pagesInPeriod, 0);
  QCOMPARE(untimed.totalPages, 0);
}

void ReadingStatisticsCalculatorTest::booksRead_missingEndPageHoldsPosition() {
  const QList<BookDTO> books{makeBook(kFirstIsbn, u"Refactoring"_s, BookStatus::InProgress, 120)};
  const QList<ReadingSessionDTO> sessions{
      makeSession(kFirstIsbn, u"2026-03-02T10:00:00"_s, 60, 0, 100),
      makeSession(kFirstIsbn, u"2026-03-03T10:00:00"_s, 60, 100, 0),
  };

  const ReadingStatisticsDTO stats =
      ReadingStatisticsCalculator::compute(books, sessions, StatisticsPeriods::week(today()));

  QCOMPARE(stats.booksRead.size(), 1);
  QCOMPARE(stats.booksRead.at(0).toPage, 100);
  QCOMPARE(stats.booksRead.at(0).pagesInPeriod, 100);
}

void ReadingStatisticsCalculatorTest::booksRead_areCappedMostRecentFirst() {
  QList<BookDTO> books;
  QList<ReadingSessionDTO> sessions;
  for (int i = 0; i < 8; ++i) {
    books.append(makeBook(i + 1, u"Book %1"_s.arg(i), BookStatus::InProgress, 10));
    sessions.append(makeSession(i + 1, u"2026-03-02T0%1:00:00"_s.arg(i), 30, 0, 10));
  }

  const ReadingStatisticsDTO stats =
      ReadingStatisticsCalculator::compute(books, sessions, StatisticsPeriods::week(today()));

  QCOMPARE(stats.booksRead.size(), ReadingStatisticsDTO::kBooksShown);
  QCOMPARE(stats.booksRead.at(0).isbn, 8);
  QCOMPARE(stats.booksRead.at(4).isbn, 4);
}

void ReadingStatisticsCalculatorTest::earliestSession_isTheFirstStart() {
  QCOMPARE(ReadingStatisticsCalculator::earliestSession(weekOfSessions()), QDate(2026, 2, 27));
  QVERIFY(!ReadingStatisticsCalculator::earliestSession({}).isValid());
}

void ReadingStatisticsCalculatorTest::twoLibrariesWithTheSameFigures_compareEqual() {
  const QList<BookDTO> books{makeBook(kFirstIsbn, u"Refactoring"_s, BookStatus::InProgress, 10)};
  const QList<ReadingSessionDTO> sessions = weekOfSessions();
  const StatisticsRange range = StatisticsPeriods::week(today());

  QCOMPARE(ReadingStatisticsCalculator::compute(books, sessions, range),
           ReadingStatisticsCalculator::compute(books, sessions, range));
}

QTEST_GUILESS_MAIN(ReadingStatisticsCalculatorTest)
#include "ReadingStatisticsCalculatorTest.moc"
