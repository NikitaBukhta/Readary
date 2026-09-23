#include "services/statistics/ReadingStatisticsCalculator.hpp"

#include "services/dto/BookStatus.hpp"

#include <QDate>
#include <QDateTime>
#include <QList>
#include <QTest>

using Qt::StringLiterals::operator""_s;

using readary::services::BookDTO;
using readary::services::BookStatus;
using readary::services::MonthlyBooksDTO;
using readary::services::ReadingSessionDTO;
using readary::services::ReadingStatisticsCalculator;
using readary::services::ReadingStatisticsDTO;

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

// Zone-less stamps, local time — the same convention BookStatisticsCalculatorTest
// follows, so the day and month a session lands on do not depend on the machine.
ReadingSessionDTO makeSession(qint64 bookIsbn, const QString &startedAt, int minutes, int pagesFrom, int pagesTo) {
  ReadingSessionDTO session;
  session.bookIsbn = bookIsbn;
  session.startedAt = QDateTime::fromString(startedAt, Qt::ISODate);
  session.endedAt = session.startedAt.addSecs(static_cast<qint64>(minutes) * 60);
  session.pagesFrom = pagesFrom;
  session.pagesTo = pagesTo;
  return session;
}

// Wednesday, 2026-03-04: the monthly series then runs October 2025..March 2026.
QDate today() { return QDate{2026, 3, 4}; }

} // namespace

class ReadingStatisticsCalculatorTest : public QObject {
  Q_OBJECT

private slots:
  void emptyLibrary_keepsTheAxesButCountsNothing();
  void journal_isSummedAcrossEveryBook();
  void weekly_bucketsEveryBookTogether();
  void booksFinished_countsTheShelfNotTheJournal();
  void monthly_datesABookByItsLastSession();
  void monthly_runsBackAcrossTheYearBoundary();
  void monthly_dropsAFinishOutsideTheWindow();
  void inProgress_listsMostRecentlyReadFirst();
  void inProgress_isCappedAndIgnoresOtherStatuses();
  void twoLibrariesWithTheSameFigures_compareEqual();
};

void ReadingStatisticsCalculatorTest::emptyLibrary_keepsTheAxesButCountsNothing() {
  const ReadingStatisticsDTO stats = ReadingStatisticsCalculator::compute({}, {}, today());

  QCOMPARE(stats.booksFinished, 0);
  QCOMPARE(stats.sessionCount, 0);
  QCOMPARE(stats.totalSeconds, 0);
  QCOMPARE(stats.weeklyPages, QList<int>(ReadingStatisticsDTO::kDaysInWeek, 0));
  QCOMPARE(stats.monthlyBooks.size(), ReadingStatisticsDTO::kMonthsShown);
  for (const MonthlyBooksDTO &month : stats.monthlyBooks) {
    QCOMPARE(month.books, 0);
  }
  QVERIFY(stats.booksInProgress.isEmpty());
}

void ReadingStatisticsCalculatorTest::journal_isSummedAcrossEveryBook() {
  // Two books, 20 p/h and 60 p/h: the speeds span both, the time adds up.
  const QList<ReadingSessionDTO> sessions{
      makeSession(kFirstIsbn, u"2026-03-03T10:00:00"_s, 60, 0, 20),
      makeSession(kSecondIsbn, u"2026-03-02T10:00:00"_s, 30, 0, 30),
  };

  const ReadingStatisticsDTO stats = ReadingStatisticsCalculator::compute({}, sessions, today());

  QCOMPARE(stats.sessionCount, 2);
  QCOMPARE(stats.timedSessionCount, 2);
  QCOMPARE(stats.totalSeconds, 90 * 60);
  QCOMPARE(stats.minPagesPerHour, 20.0);
  QCOMPARE(stats.maxPagesPerHour, 60.0);
  // Duration-weighted: 50 pages over 1.5 hours.
  QCOMPARE(qRound(stats.averagePagesPerHour * 100), qRound(50.0 / 1.5 * 100));
}

void ReadingStatisticsCalculatorTest::weekly_bucketsEveryBookTogether() {
  const QList<ReadingSessionDTO> sessions{
      makeSession(kFirstIsbn, u"2026-03-02T10:00:00"_s, 30, 0, 25),   // Monday
      makeSession(kSecondIsbn, u"2026-03-02T20:00:00"_s, 30, 10, 25), // Monday
      makeSession(kSecondIsbn, u"2026-03-08T09:00:00"_s, 30, 25, 40), // Sunday
      makeSession(kFirstIsbn, u"2026-02-27T10:00:00"_s, 30, 0, 99),   // week before
  };

  const ReadingStatisticsDTO stats = ReadingStatisticsCalculator::compute({}, sessions, today());

  QCOMPARE(stats.weeklyPages, (QList<int>{40, 0, 0, 0, 0, 0, 15}));
}

void ReadingStatisticsCalculatorTest::booksFinished_countsTheShelfNotTheJournal() {
  // A book marked finished by hand was never timed, and still counts.
  const QList<BookDTO> books{
      makeBook(kFirstIsbn, u"Refactoring"_s, BookStatus::Finished, 300),
      makeBook(kSecondIsbn, u"Effective Modern C++"_s, BookStatus::InProgress, 40),
      makeBook(kThirdIsbn, u"Clean Architecture"_s, BookStatus::Finished, 0),
  };

  const ReadingStatisticsDTO stats = ReadingStatisticsCalculator::compute(books, {}, today());

  QCOMPARE(stats.booksFinished, 2);
  // Neither finish has a date, so no month claims it.
  for (const MonthlyBooksDTO &month : stats.monthlyBooks) {
    QCOMPARE(month.books, 0);
  }
}

void ReadingStatisticsCalculatorTest::monthly_datesABookByItsLastSession() {
  const QList<BookDTO> books{
      makeBook(kFirstIsbn, u"Refactoring"_s, BookStatus::Finished),
      makeBook(kSecondIsbn, u"Effective Modern C++"_s, BookStatus::Finished),
      // Read in February but not finished: no month for it.
      makeBook(kThirdIsbn, u"Clean Architecture"_s, BookStatus::InProgress),
  };
  const QList<ReadingSessionDTO> sessions{
      // Started in January, finished in February — February is what counts.
      makeSession(kFirstIsbn, u"2026-01-20T10:00:00"_s, 60, 0, 150),
      makeSession(kFirstIsbn, u"2026-02-10T10:00:00"_s, 60, 150, 300),
      makeSession(kSecondIsbn, u"2026-02-25T10:00:00"_s, 60, 0, 300),
      makeSession(kThirdIsbn, u"2026-02-26T10:00:00"_s, 60, 0, 50),
  };

  const ReadingStatisticsDTO stats = ReadingStatisticsCalculator::compute(books, sessions, today());

  QCOMPARE(stats.monthlyBooks.size(), ReadingStatisticsDTO::kMonthsShown);
  QCOMPARE(stats.monthlyBooks.at(4), (MonthlyBooksDTO{.year = 2026, .month = 2, .books = 2}));
  QCOMPARE(stats.monthlyBooks.at(3), (MonthlyBooksDTO{.year = 2026, .month = 1, .books = 0}));
  QCOMPARE(stats.monthlyBooks.at(5), (MonthlyBooksDTO{.year = 2026, .month = 3, .books = 0}));
}

void ReadingStatisticsCalculatorTest::monthly_runsBackAcrossTheYearBoundary() {
  const ReadingStatisticsDTO stats = ReadingStatisticsCalculator::compute({}, {}, today());

  const QList<MonthlyBooksDTO> expected{
      {.year = 2025, .month = 10, .books = 0}, {.year = 2025, .month = 11, .books = 0},
      {.year = 2025, .month = 12, .books = 0}, {.year = 2026, .month = 1, .books = 0},
      {.year = 2026, .month = 2, .books = 0},  {.year = 2026, .month = 3, .books = 0},
  };
  QCOMPARE(stats.monthlyBooks, expected);
}

void ReadingStatisticsCalculatorTest::monthly_dropsAFinishOutsideTheWindow() {
  const QList<BookDTO> books{
      makeBook(kFirstIsbn, u"Refactoring"_s, BookStatus::Finished),
      makeBook(kSecondIsbn, u"Effective Modern C++"_s, BookStatus::Finished),
  };
  const QList<ReadingSessionDTO> sessions{
      makeSession(kFirstIsbn, u"2025-09-30T10:00:00"_s, 60, 0, 300),  // a month too early
      makeSession(kSecondIsbn, u"2025-10-01T10:00:00"_s, 60, 0, 300), // first month shown
  };

  const ReadingStatisticsDTO stats = ReadingStatisticsCalculator::compute(books, sessions, today());

  QCOMPARE(stats.booksFinished, 2);
  QCOMPARE(stats.monthlyBooks.at(0).books, 1);
  int total = 0;
  for (const MonthlyBooksDTO &month : stats.monthlyBooks) {
    total += month.books;
  }
  QCOMPARE(total, 1);
}

void ReadingStatisticsCalculatorTest::inProgress_listsMostRecentlyReadFirst() {
  const QList<BookDTO> books{
      makeBook(kFirstIsbn, u"Zebra"_s, BookStatus::InProgress, 10, 100),
      makeBook(kSecondIsbn, u"Refactoring"_s, BookStatus::InProgress, 120, 300),
      // Never timed: after every timed book, by name.
      makeBook(kThirdIsbn, u"Alpha"_s, BookStatus::InProgress, 5, 0),
  };
  const QList<ReadingSessionDTO> sessions{
      makeSession(kFirstIsbn, u"2026-02-01T10:00:00"_s, 30, 0, 10),
      makeSession(kSecondIsbn, u"2026-03-01T10:00:00"_s, 30, 100, 120),
  };

  const ReadingStatisticsDTO stats = ReadingStatisticsCalculator::compute(books, sessions, today());

  QCOMPARE(stats.booksInProgress.size(), 3);
  QCOMPARE(stats.booksInProgress.at(0).isbn, kSecondIsbn);
  QCOMPARE(stats.booksInProgress.at(0).pagesRead, 120);
  QCOMPARE(stats.booksInProgress.at(0).totalPages, 300);
  QCOMPARE(stats.booksInProgress.at(1).isbn, kFirstIsbn);
  QCOMPARE(stats.booksInProgress.at(2).name, u"Alpha"_s);
  QCOMPARE(stats.booksInProgress.at(2).totalPages, 0);
}

void ReadingStatisticsCalculatorTest::inProgress_isCappedAndIgnoresOtherStatuses() {
  QList<BookDTO> books{
      makeBook(kFirstIsbn, u"Finished"_s, BookStatus::Finished, 300),
      makeBook(kSecondIsbn, u"Someday"_s, BookStatus::WantToRead),
  };
  for (int i = 0; i < 8; ++i) {
    books.append(makeBook(i + 1, u"Open %1"_s.arg(i), BookStatus::InProgress, i));
  }

  const ReadingStatisticsDTO stats = ReadingStatisticsCalculator::compute(books, {}, today());

  QCOMPARE(stats.booksInProgress.size(), ReadingStatisticsDTO::kBooksInProgressShown);
  QCOMPARE(stats.booksInProgress.at(0).name, u"Open 0"_s);
}

void ReadingStatisticsCalculatorTest::twoLibrariesWithTheSameFigures_compareEqual() {
  // The controller drops a recompute that changed nothing, which rests on this.
  const QList<BookDTO> books{makeBook(kFirstIsbn, u"Refactoring"_s, BookStatus::InProgress, 10)};
  const QList<ReadingSessionDTO> sessions{makeSession(kFirstIsbn, u"2026-03-02T10:00:00"_s, 30, 0, 10)};

  QCOMPARE(ReadingStatisticsCalculator::compute(books, sessions, today()),
           ReadingStatisticsCalculator::compute(books, sessions, today()));
}

QTEST_GUILESS_MAIN(ReadingStatisticsCalculatorTest)
#include "ReadingStatisticsCalculatorTest.moc"
