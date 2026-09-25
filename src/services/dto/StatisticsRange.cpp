#include "services/dto/StatisticsRange.hpp"

#include <QCalendar>

#include <chrono>

namespace {

int monthsBetween(QDate from, QDate to) {
  return ((to.year() - from.year()) * QCalendar{}.maximumMonthsInYear()) + to.month() - from.month();
}

} // namespace

namespace readary::services {

bool StatisticsRange::contains(QDate date) const { return date.isValid() && date >= from && date <= to; }

int StatisticsRange::bucketCount() const {
  if (!from.isValid() || !to.isValid() || to < from) {
    return 0;
  }
  switch (granularity) {
  case StatisticsGranularity::Hour:
    return static_cast<int>(std::chrono::days{1} / std::chrono::hours{1});
  case StatisticsGranularity::Day:
    return static_cast<int>(from.daysTo(to)) + 1;
  case StatisticsGranularity::Month:
    return monthsBetween(from, to) + 1;
  case StatisticsGranularity::Year:
    return to.year() - from.year() + 1;
  case StatisticsGranularity::Count:
    break;
  }
  return 0;
}

int StatisticsRange::bucketOf(const QDateTime &moment) const {
  const QDate date = moment.date();
  if (!moment.isValid() || !contains(date)) {
    return -1;
  }
  switch (granularity) {
  case StatisticsGranularity::Hour:
    return moment.time().hour();
  case StatisticsGranularity::Day:
    return static_cast<int>(from.daysTo(date));
  case StatisticsGranularity::Month:
    return monthsBetween(from, date);
  case StatisticsGranularity::Year:
    return date.year() - from.year();
  case StatisticsGranularity::Count:
    break;
  }
  return -1;
}

QDate StatisticsRange::bucketStart(int index) const {
  if (index < 0 || index >= bucketCount()) {
    return {};
  }
  switch (granularity) {
  case StatisticsGranularity::Hour:
    return from;
  case StatisticsGranularity::Day:
    return from.addDays(index);
  case StatisticsGranularity::Month:
    return QDate{from.year(), from.month(), 1}.addMonths(index);
  case StatisticsGranularity::Year:
    return QDate{from.year() + index, 1, 1};
  case StatisticsGranularity::Count:
    break;
  }
  return {};
}

} // namespace readary::services
