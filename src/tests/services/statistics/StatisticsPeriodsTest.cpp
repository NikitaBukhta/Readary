#include "services/statistics/StatisticsPeriods.hpp"

#include <QDate>
#include <QDateTime>
#include <QTest>

using readary::services::StatisticsGranularity;
using readary::services::StatisticsPeriods;
using readary::services::StatisticsRange;

Q_DECLARE_METATYPE(readary::services::StatisticsGranularity)

namespace {

QDate today() { return QDate{2026, 3, 4}; }

} // namespace

class StatisticsPeriodsTest : public QObject {
  Q_OBJECT

private slots:
  void day_isTodayByTheHour();
  void week_runsMondayToSunday();
  void month_isTheWholeCalendarMonth();
  void year_isTheWholeCalendarYearByMonth();
  void year_coversTheYearOfAnyDay_data();
  void year_coversTheYearOfAnyDay();
  void allTime_keepsAtLeastSixMonths();
  void allTime_reachesBackToTheFirstSession();
  void allTime_switchesToYearsPastTwoYears();
  void custom_putsReversedEndsInOrder();
  void custom_granularityFollowsTheLength_data();
  void custom_granularityFollowsTheLength();
};

void StatisticsPeriodsTest::day_isTodayByTheHour() {
  const StatisticsRange range = StatisticsPeriods::day(today());

  QCOMPARE(range.from, today());
  QCOMPARE(range.to, today());
  QCOMPARE(range.granularity, StatisticsGranularity::Hour);
  QVERIFY(!range.unbounded);
}

void StatisticsPeriodsTest::week_runsMondayToSunday() {
  const StatisticsRange range = StatisticsPeriods::week(today());

  QCOMPARE(range.from, QDate(2026, 3, 2));
  QCOMPARE(range.to, QDate(2026, 3, 8));
  QCOMPARE(range.granularity, StatisticsGranularity::Day);
}

void StatisticsPeriodsTest::month_isTheWholeCalendarMonth() {
  const StatisticsRange range = StatisticsPeriods::month(today());

  QCOMPARE(range.from, QDate(2026, 3, 1));
  QCOMPARE(range.to, QDate(2026, 3, 31));
  QCOMPARE(range.granularity, StatisticsGranularity::Day);
}

void StatisticsPeriodsTest::year_isTheWholeCalendarYearByMonth() {
  const StatisticsRange range = StatisticsPeriods::year(today());

  QCOMPARE(range.from, QDate(2026, 1, 1));
  QCOMPARE(range.to, QDate(2026, 12, 31));
  QCOMPARE(range.granularity, StatisticsGranularity::Month);
  QCOMPARE(range.bucketCount(), 12);
}

void StatisticsPeriodsTest::year_coversTheYearOfAnyDay_data() {
  QTest::addColumn<QDate>("day");

  QTest::newRow("first day") << QDate{2026, 1, 1};
  QTest::newRow("last day") << QDate{2026, 12, 31};
  QTest::newRow("leap day") << QDate{2028, 2, 29};
}

void StatisticsPeriodsTest::year_coversTheYearOfAnyDay() {
  QFETCH(QDate, day);

  const StatisticsRange range = StatisticsPeriods::year(day);

  QCOMPARE(range.from, QDate(day.year(), 1, 1));
  QCOMPARE(range.to, QDate(day.year(), 12, 31));
  QVERIFY(range.contains(day));
  QCOMPARE(range.bucketCount(), 12);
  QCOMPARE(range.bucketOf(QDateTime{day, QTime{12, 0}}), day.month() - 1);
  QCOMPARE(range.bucketStart(11), QDate(day.year(), 12, 1));
}

void StatisticsPeriodsTest::allTime_keepsAtLeastSixMonths() {
  const StatisticsRange range = StatisticsPeriods::allTime(today(), QDate{2026, 3, 2});

  QCOMPARE(range.from, QDate(2025, 10, 1));
  QCOMPARE(range.to, QDate(2026, 3, 31));
  QCOMPARE(range.granularity, StatisticsGranularity::Month);
  QCOMPARE(range.bucketCount(), 6);
  QVERIFY(range.unbounded);

  QCOMPARE(StatisticsPeriods::allTime(today(), {}).from, QDate(2025, 10, 1));
}

void StatisticsPeriodsTest::allTime_reachesBackToTheFirstSession() {
  const StatisticsRange range = StatisticsPeriods::allTime(today(), QDate{2025, 1, 20});

  QCOMPARE(range.from, QDate(2025, 1, 1));
  QCOMPARE(range.bucketCount(), 15);
}

void StatisticsPeriodsTest::allTime_switchesToYearsPastTwoYears() {
  const StatisticsRange range = StatisticsPeriods::allTime(today(), QDate{2022, 6, 15});

  QCOMPARE(range.granularity, StatisticsGranularity::Year);
  QCOMPARE(range.from, QDate(2022, 1, 1));
  QCOMPARE(range.to, QDate(2026, 12, 31));
  QCOMPARE(range.bucketCount(), 5);
}

void StatisticsPeriodsTest::custom_putsReversedEndsInOrder() {
  const StatisticsRange range = StatisticsPeriods::custom(QDate{2026, 2, 20}, QDate{2026, 2, 10});

  QCOMPARE(range.from, QDate(2026, 2, 10));
  QCOMPARE(range.to, QDate(2026, 2, 20));
  QCOMPARE(range.bucketCount(), 11);
}

void StatisticsPeriodsTest::custom_granularityFollowsTheLength_data() {
  QTest::addColumn<QDate>("from");
  QTest::addColumn<QDate>("to");
  QTest::addColumn<StatisticsGranularity>("granularity");
  QTest::addColumn<int>("buckets");

  QTest::newRow("one day") << QDate{2026, 2, 10} << QDate{2026, 2, 10} << StatisticsGranularity::Hour << 24;
  QTest::newRow("two days") << QDate{2026, 2, 10} << QDate{2026, 2, 11} << StatisticsGranularity::Day << 2;

  const QDate start{2026, 1, 1};
  const QDate lastDaily = start.addDays(StatisticsPeriods::kMaxDayBuckets - 1);
  QTest::newRow("most days") << start << lastDaily << StatisticsGranularity::Day << StatisticsPeriods::kMaxDayBuckets;
  QTest::newRow("one day more") << start << lastDaily.addDays(1) << StatisticsGranularity::Month
                                << lastDaily.addDays(1).month() - start.month() + 1;

  const QDate lastMonthly = start.addMonths(StatisticsPeriods::kMaxMonthBuckets - 1);
  QTest::newRow("most months") << start << lastMonthly << StatisticsGranularity::Month
                               << StatisticsPeriods::kMaxMonthBuckets;
  QTest::newRow("one month more") << start << lastMonthly.addMonths(1) << StatisticsGranularity::Year
                                  << lastMonthly.addMonths(1).year() - start.year() + 1;
}

void StatisticsPeriodsTest::custom_granularityFollowsTheLength() {
  QFETCH(QDate, from);
  QFETCH(QDate, to);
  QFETCH(StatisticsGranularity, granularity);
  QFETCH(int, buckets);

  const StatisticsRange range = StatisticsPeriods::custom(from, to);

  QCOMPARE(range.granularity, granularity);
  QCOMPARE(range.bucketCount(), buckets);
  QVERIFY(!range.unbounded);
}

QTEST_GUILESS_MAIN(StatisticsPeriodsTest)
#include "StatisticsPeriodsTest.moc"
