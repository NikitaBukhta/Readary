#include "services/statistics/BookStatisticsCalculator.hpp"
#include "services/dto/BookStatisticsDTO.hpp"
#include "services/dto/ReadingSessionDTO.hpp"

#include <QDate>
#include <QDateTime>
#include <QList>
#include <QTest>

using Qt::StringLiterals::operator""_s;

using readary::services::BookStatisticsCalculator;
using readary::services::BookStatisticsDTO;
using readary::services::ReadingSessionDTO;

using Sessions = QList<ReadingSessionDTO>;
Q_DECLARE_METATYPE(Sessions)

namespace {

// Zone-less stamps on purpose: ReadingSessionDTO hands the journal's UTC
// timestamps over as local time, so the weekday a session lands on is a local
// one. A fixture written in local time buckets the same way on any machine.
ReadingSessionDTO makeSession(const QString &startedAt, int minutes, int pagesFrom, int pagesTo) {
  ReadingSessionDTO session;
  session.startedAt = QDateTime::fromString(startedAt, Qt::ISODate);
  session.endedAt = session.startedAt.addSecs(static_cast<qint64>(minutes) * 60);
  session.pagesFrom = pagesFrom;
  session.pagesTo = pagesTo;
  return session;
}

// Mon 2026-03-02, Wed 2026-03-04 and Sat 2026-03-07, newest first the way
// BookTable returns them. 110 pages over 3 hours, at 30..40 pages an hour.
Sessions threeSessions() {
  return {
      makeSession(u"2026-03-07T09:00:00"_s, 90, 50, 110),
      makeSession(u"2026-03-04T20:00:00"_s, 30, 30, 50),
      makeSession(u"2026-03-02T10:00:00"_s, 60, 0, 30),
  };
}

// Wednesday of the week the fixture sessions fall in.
QDate weekReference() { return QDate{2026, 3, 4}; }

} // namespace

class BookStatisticsCalculatorTest : public QObject {
  Q_OBJECT

private slots:
  void totals_sumPagesAndDurationAcrossSessions();
  void emptyJournal_yieldsZeroTotals();

  void speed_data();
  void speed();

  void weekly_data();
  void weekly();

  void progress_walksTheSessionsOldestFirst();
  void progress_breaksTiesOnInsertionOrder();
  void progress_isEmptyWithoutSessions();
  void progress_holdsThePositionWhenASessionLoggedNoEndPage();
};

void BookStatisticsCalculatorTest::totals_sumPagesAndDurationAcrossSessions() {
  const BookStatisticsDTO stats = BookStatisticsCalculator::compute(threeSessions(), weekReference());

  QCOMPARE(stats.sessionCount, 3);
  QCOMPARE(stats.pagesRead, 110);
  QCOMPARE(stats.totalSeconds, 3 * 60 * 60);
}

void BookStatisticsCalculatorTest::emptyJournal_yieldsZeroTotals() {
  const BookStatisticsDTO stats = BookStatisticsCalculator::compute({}, weekReference());

  QCOMPARE(stats.sessionCount, 0);
  QCOMPARE(stats.pagesRead, 0);
  QCOMPARE(stats.totalSeconds, 0);
  QCOMPARE(stats.averagePagesPerHour, 0.0);
}

// All three speeds come from one set — the sessions that were timed and
// logged an end page — so the average is their duration-weighted mean and can
// never leave min..max, which is what the card prints it between.
void BookStatisticsCalculatorTest::speed_data() {
  QTest::addColumn<Sessions>("sessions");
  QTest::addColumn<int>("expectedTimedCount");
  QTest::addColumn<double>("expectedMinimum");
  QTest::addColumn<double>("expectedAverage");
  QTest::addColumn<double>("expectedMaximum");

  QTest::newRow("spans the slowest and fastest session") << threeSessions() << 3 << 30.0 << (110.0 / 3.0) << 40.0;

  // Saved the instant it started: no duration to divide by, so no speed.
  QTest::newRow("ignores a session that took no time")
      << Sessions{makeSession(u"2026-03-02T10:00:00"_s, 60, 0, 30), makeSession(u"2026-03-03T10:00:00"_s, 0, 30, 45)}
      << 1 << 30.0 << 30.0 << 30.0;

  // Forty-five minutes without turning a page really is 0 p/h — the reader's
  // slowest session, not a measurement error.
  QTest::newRow("counts a timed session that gained no page")
      << Sessions{makeSession(u"2026-03-02T10:00:00"_s, 60, 0, 30), makeSession(u"2026-03-03T10:00:00"_s, 45, 30, 30)}
      << 2 << 0.0 << (30.0 / (105.0 / 60.0)) << 30.0;

  // pages_to NULL reads back as 0, which is a missing measurement rather than
  // zero pages; the progress curve holds position for the same row.
  QTest::newRow("skips a session that logged no end page")
      << Sessions{makeSession(u"2026-03-02T10:00:00"_s, 60, 0, 120), makeSession(u"2026-03-03T10:00:00"_s, 30, 120, 0)}
      << 1 << 120.0 << 120.0 << 120.0;

  QTest::newRow("one session is that session everywhere")
      << Sessions{makeSession(u"2026-03-02T10:00:00"_s, 30, 0, 20)} << 1 << 40.0 << 40.0 << 40.0;

  QTest::newRow("nothing timed leaves every speed at zero")
      << Sessions{makeSession(u"2026-03-02T10:00:00"_s, 0, 10, 10)} << 0 << 0.0 << 0.0 << 0.0;

  QTest::newRow("empty journal") << Sessions{} << 0 << 0.0 << 0.0 << 0.0;
}

void BookStatisticsCalculatorTest::speed() {
  QFETCH(Sessions, sessions);
  QFETCH(int, expectedTimedCount);
  QFETCH(double, expectedMinimum);
  QFETCH(double, expectedAverage);
  QFETCH(double, expectedMaximum);

  const BookStatisticsDTO stats = BookStatisticsCalculator::compute(sessions, weekReference());

  QCOMPARE(stats.timedSessionCount, expectedTimedCount);
  QCOMPARE(stats.minPagesPerHour, expectedMinimum);
  QCOMPARE(stats.averagePagesPerHour, expectedAverage);
  QCOMPARE(stats.maxPagesPerHour, expectedMaximum);
  QVERIFY(stats.averagePagesPerHour >= stats.minPagesPerHour);
  QVERIFY(stats.averagePagesPerHour <= stats.maxPagesPerHour);
}

void BookStatisticsCalculatorTest::weekly_data() {
  QTest::addColumn<Sessions>("sessions");
  QTest::addColumn<QDate>("reference");
  QTest::addColumn<QList<int>>("expectedBuckets");

  // Mon 02.03 gained 30 pages, Wed 04.03 gained 20, Sat 07.03 gained 60.
  QTest::newRow("buckets by weekday") << threeSessions() << weekReference() << QList<int>{30, 0, 20, 0, 0, 60, 0};

  Sessions spanningWeeks = threeSessions();
  spanningWeeks.append(makeSession(u"2026-02-28T10:00:00"_s, 60, 200, 290)); // Sat, week before
  spanningWeeks.append(makeSession(u"2026-03-09T10:00:00"_s, 60, 290, 380)); // Mon, week after
  QTest::newRow("excludes other weeks") << spanningWeeks << weekReference() << QList<int>{30, 0, 20, 0, 0, 60, 0};

  QTest::newRow("nothing read keeps seven buckets") << Sessions{} << weekReference() << QList<int>{0, 0, 0, 0, 0, 0, 0};

  // Sunday closes the week here, so a Sunday reference must not roll the
  // window forward onto the next Monday.
  QTest::newRow("sunday is the last bucket") << Sessions{makeSession(u"2026-03-08T21:00:00"_s, 60, 0, 25)}
                                             << QDate{2026, 3, 8} << QList<int>{0, 0, 0, 0, 0, 0, 25};
}

void BookStatisticsCalculatorTest::weekly() {
  QFETCH(Sessions, sessions);
  QFETCH(QDate, reference);
  QFETCH(QList<int>, expectedBuckets);

  QCOMPARE(BookStatisticsCalculator::compute(sessions, reference).weeklyPages, expectedBuckets);
}

void BookStatisticsCalculatorTest::progress_walksTheSessionsOldestFirst() {
  // BookTable hands the journal over newest first; the curve reads left to
  // right, so the calculator has to turn it around itself.
  const BookStatisticsDTO stats = BookStatisticsCalculator::compute(threeSessions(), weekReference());

  QCOMPARE(stats.progressPoints.size(), 3);
  QCOMPARE(stats.progressPoints.at(0).session, 1);
  QCOMPARE(stats.progressPoints.at(0).page, 30);
  QCOMPARE(stats.progressPoints.at(1).session, 2);
  QCOMPARE(stats.progressPoints.at(1).page, 50);
  QCOMPARE(stats.progressPoints.at(2).session, 3);
  QCOMPARE(stats.progressPoints.at(2).page, 110);
}

void BookStatisticsCalculatorTest::progress_breaksTiesOnInsertionOrder() {
  // Two sessions stamped the same second (the timer writes whole seconds, so a
  // pair of short sessions can collide). The row id is the only thing left
  // that says which came first.
  ReadingSessionDTO first = makeSession(u"2026-03-02T10:00:00"_s, 10, 0, 12);
  first.id = 7;
  ReadingSessionDTO second = makeSession(u"2026-03-02T10:00:00"_s, 10, 12, 20);
  second.id = 8;

  const BookStatisticsDTO stats = BookStatisticsCalculator::compute({second, first}, weekReference());

  QCOMPARE(stats.progressPoints.size(), 2);
  QCOMPARE(stats.progressPoints.at(0).page, 12);
  QCOMPARE(stats.progressPoints.at(1).page, 20);
}

void BookStatisticsCalculatorTest::progress_isEmptyWithoutSessions() {
  QVERIFY(BookStatisticsCalculator::compute({}, weekReference()).progressPoints.isEmpty());
}

void BookStatisticsCalculatorTest::progress_holdsThePositionWhenASessionLoggedNoEndPage() {
  // Plotting the NULL literally drops the curve to the baseline and back; the
  // reader did not un-read 120 pages, so the point holds position.
  const Sessions sessions{
      makeSession(u"2026-03-02T10:00:00"_s, 60, 0, 120),
      makeSession(u"2026-03-03T10:00:00"_s, 30, 120, 0),
  };

  const BookStatisticsDTO stats = BookStatisticsCalculator::compute(sessions, weekReference());

  QCOMPARE(stats.progressPoints.size(), 2);
  QCOMPARE(stats.progressPoints.at(0).page, 120);
  QCOMPARE(stats.progressPoints.at(1).page, 120);
}

QTEST_GUILESS_MAIN(BookStatisticsCalculatorTest)
#include "BookStatisticsCalculatorTest.moc"
