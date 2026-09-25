#ifndef READARY_SERVICES_STATISTICSPERIODS_HPP
#define READARY_SERVICES_STATISTICSPERIODS_HPP

#include "services/dto/StatisticsRange.hpp"

#include <QDate>

namespace readary::services {

class StatisticsPeriods {
public:
  static constexpr int kMinAllTimeMonths = 6;
  static constexpr int kMaxDayBuckets = 62;
  static constexpr int kMaxMonthBuckets = 24;

  static StatisticsRange day(QDate today);
  static StatisticsRange week(QDate today);
  static StatisticsRange month(QDate today);
  static StatisticsRange year(QDate today);
  static StatisticsRange allTime(QDate today, QDate earliestSession);
  static StatisticsRange custom(QDate from, QDate to);
};

} // namespace readary::services

#endif // READARY_SERVICES_STATISTICSPERIODS_HPP
