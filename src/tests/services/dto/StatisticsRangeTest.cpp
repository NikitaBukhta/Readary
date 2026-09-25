#include "services/dto/StatisticsRange.hpp"

#include <QDate>
#include <QTest>

using readary::services::StatisticsGranularity;
using readary::services::StatisticsRange;

Q_DECLARE_METATYPE(readary::services::StatisticsGranularity)

namespace {

StatisticsRange range(QDate from, QDate to, StatisticsGranularity granularity) {
  return {.from = from, .to = to, .granularity = granularity};
}

QDateTime at(QDate date, int hour) { return QDateTime{date, QTime{hour, 30}}; }

} // namespace

class StatisticsRangeTest : public QObject {
  Q_OBJECT

private slots:
  void contains_isInclusiveAtBothEnds();
  void bucketCount_followsTheGranularity_data();
  void bucketCount_followsTheGranularity();
  void bucketCount_ofAnInvertedRange_isZero();
  void bucketOf_placesAMomentInItsBucket();
  void bucketOf_outsideTheRange_isMinusOne();
  void bucketStart_isTheFirstDayOfEachBucket();
  void bucketStart_andBucketOf_agree();
};

void StatisticsRangeTest::contains_isInclusiveAtBothEnds() {
  const StatisticsRange week = range({2026, 3, 2}, {2026, 3, 8}, StatisticsGranularity::Day);

  QVERIFY(week.contains({2026, 3, 2}));
  QVERIFY(week.contains({2026, 3, 8}));
  QVERIFY(!week.contains({2026, 3, 1}));
  QVERIFY(!week.contains({2026, 3, 9}));
  QVERIFY(!week.contains({}));
}

void StatisticsRangeTest::bucketCount_followsTheGranularity_data() {
  QTest::addColumn<QDate>("from");
  QTest::addColumn<QDate>("to");
  QTest::addColumn<StatisticsGranularity>("granularity");
  QTest::addColumn<int>("buckets");

  QTest::newRow("hours") << QDate{2026, 3, 4} << QDate{2026, 3, 4} << StatisticsGranularity::Hour << 24;
  QTest::newRow("days") << QDate{2026, 3, 1} << QDate{2026, 3, 31} << StatisticsGranularity::Day << 31;
  QTest::newRow("months across a year") << QDate{2025, 10, 15} << QDate{2026, 3, 4} << StatisticsGranularity::Month
                                        << 6;
  QTest::newRow("years") << QDate{2022, 1, 1} << QDate{2026, 12, 31} << StatisticsGranularity::Year << 5;
}

void StatisticsRangeTest::bucketCount_followsTheGranularity() {
  QFETCH(QDate, from);
  QFETCH(QDate, to);
  QFETCH(StatisticsGranularity, granularity);
  QFETCH(int, buckets);

  QCOMPARE(range(from, to, granularity).bucketCount(), buckets);
}

void StatisticsRangeTest::bucketCount_ofAnInvertedRange_isZero() {
  QCOMPARE(range({2026, 3, 8}, {2026, 3, 2}, StatisticsGranularity::Day).bucketCount(), 0);
  QCOMPARE(range({}, {}, StatisticsGranularity::Day).bucketCount(), 0);
}

void StatisticsRangeTest::bucketOf_placesAMomentInItsBucket() {
  QCOMPARE(range({2026, 3, 4}, {2026, 3, 4}, StatisticsGranularity::Hour).bucketOf(at({2026, 3, 4}, 21)), 21);
  QCOMPARE(range({2026, 3, 2}, {2026, 3, 8}, StatisticsGranularity::Day).bucketOf(at({2026, 3, 8}, 9)), 6);
  QCOMPARE(range({2026, 1, 1}, {2026, 12, 31}, StatisticsGranularity::Month).bucketOf(at({2026, 11, 2}, 9)), 10);
  QCOMPARE(range({2022, 1, 1}, {2026, 12, 31}, StatisticsGranularity::Year).bucketOf(at({2024, 7, 1}, 9)), 2);
}

void StatisticsRangeTest::bucketOf_outsideTheRange_isMinusOne() {
  const StatisticsRange week = range({2026, 3, 2}, {2026, 3, 8}, StatisticsGranularity::Day);

  QCOMPARE(week.bucketOf(at({2026, 3, 1}, 23)), -1);
  QCOMPARE(week.bucketOf(at({2026, 3, 9}, 0)), -1);
  QCOMPARE(week.bucketOf(QDateTime{}), -1);
}

void StatisticsRangeTest::bucketStart_isTheFirstDayOfEachBucket() {
  const StatisticsRange months = range({2025, 10, 15}, {2026, 3, 4}, StatisticsGranularity::Month);
  QCOMPARE(months.bucketStart(0), QDate(2025, 10, 1));
  QCOMPARE(months.bucketStart(3), QDate(2026, 1, 1));
  QCOMPARE(months.bucketStart(5), QDate(2026, 3, 1));
  QVERIFY(!months.bucketStart(6).isValid());
  QVERIFY(!months.bucketStart(-1).isValid());

  QCOMPARE(range({2026, 3, 4}, {2026, 3, 4}, StatisticsGranularity::Hour).bucketStart(23), QDate(2026, 3, 4));
  QCOMPARE(range({2026, 3, 2}, {2026, 3, 8}, StatisticsGranularity::Day).bucketStart(6), QDate(2026, 3, 8));
  QCOMPARE(range({2022, 1, 1}, {2026, 12, 31}, StatisticsGranularity::Year).bucketStart(4), QDate(2026, 1, 1));
}

void StatisticsRangeTest::bucketStart_andBucketOf_agree() {
  for (const StatisticsRange &r : {range({2026, 3, 1}, {2026, 3, 31}, StatisticsGranularity::Day),
                                   range({2024, 11, 1}, {2026, 3, 31}, StatisticsGranularity::Month),
                                   range({2020, 1, 1}, {2026, 12, 31}, StatisticsGranularity::Year)}) {
    for (int i = 0; i < r.bucketCount(); ++i) {
      QCOMPARE(r.bucketOf(QDateTime{r.bucketStart(i), QTime{12, 0}}), i);
    }
  }
}

QTEST_GUILESS_MAIN(StatisticsRangeTest)
#include "StatisticsRangeTest.moc"
