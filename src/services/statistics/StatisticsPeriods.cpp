#include "services/statistics/StatisticsPeriods.hpp"

#include <algorithm>
#include <limits>

namespace {

using readary::services::StatisticsGranularity;
using readary::services::StatisticsPeriods;
using readary::services::StatisticsRange;

int maxBuckets(StatisticsGranularity granularity) {
  switch (granularity) {
  case StatisticsGranularity::Day:
    return StatisticsPeriods::kMaxDayBuckets;
  case StatisticsGranularity::Month:
    return StatisticsPeriods::kMaxMonthBuckets;
  case StatisticsGranularity::Hour:
  case StatisticsGranularity::Year:
  case StatisticsGranularity::Count:
    break;
  }
  return std::numeric_limits<int>::max();
}

StatisticsGranularity coarser(StatisticsGranularity granularity) {
  switch (granularity) {
  case StatisticsGranularity::Hour:
    return StatisticsGranularity::Day;
  case StatisticsGranularity::Day:
    return StatisticsGranularity::Month;
  case StatisticsGranularity::Month:
  case StatisticsGranularity::Year:
  case StatisticsGranularity::Count:
    break;
  }
  return StatisticsGranularity::Year;
}

StatisticsRange finestFitting(QDate from, QDate to, StatisticsGranularity finest) {
  StatisticsRange range{.from = from, .to = to, .granularity = finest};
  while (range.bucketCount() > maxBuckets(range.granularity)) {
    range.granularity = coarser(range.granularity);
  }
  return range;
}

} // namespace

namespace readary::services {

StatisticsRange StatisticsPeriods::day(QDate today) {
  return {.from = today, .to = today, .granularity = StatisticsGranularity::Hour};
}

StatisticsRange StatisticsPeriods::week(QDate today) {
  const QDate monday = today.addDays(1 - today.dayOfWeek());
  return {.from = monday, .to = monday.addDays(6), .granularity = StatisticsGranularity::Day};
}

StatisticsRange StatisticsPeriods::month(QDate today) {
  return {.from = QDate{today.year(), today.month(), 1},
          .to = QDate{today.year(), today.month(), today.daysInMonth()},
          .granularity = StatisticsGranularity::Day};
}

StatisticsRange StatisticsPeriods::year(QDate today) {
  return {
      .from = QDate{today.year(), 1, 1},
      .to = QDate{today.year(), 12, 31},
      .granularity = StatisticsGranularity::Month,
  };
}

StatisticsRange StatisticsPeriods::allTime(QDate today, QDate earliestSession) {
  const QDate start = earliestSession.isValid() && earliestSession < today ? earliestSession : today;
  const QDate from = std::min(month(start).from, month(today).from.addMonths(1 - kMinAllTimeMonths));
  StatisticsRange range = finestFitting(from, month(today).to, StatisticsGranularity::Month);
  if (range.granularity == StatisticsGranularity::Year) {
    range.from = year(from).from;
    range.to = year(today).to;
  }
  range.unbounded = true;
  return range;
}

StatisticsRange StatisticsPeriods::custom(QDate from, QDate to) {
  const auto [first, last] = std::minmax(from, to);
  return first == last ? day(first) : finestFitting(first, last, StatisticsGranularity::Day);
}

} // namespace readary::services
