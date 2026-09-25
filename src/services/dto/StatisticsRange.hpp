#ifndef READARY_SERVICES_STATISTICSRANGE_HPP
#define READARY_SERVICES_STATISTICSRANGE_HPP

#include <QDateTime>

#include <cstdint>

namespace readary::services {

enum class StatisticsGranularity : std::uint8_t {
  Hour,
  Day,
  Month,
  Year,
  Count,
};

struct StatisticsRange {
  QDate from;
  QDate to;
  StatisticsGranularity granularity = StatisticsGranularity::Day;
  bool unbounded = false;

  bool contains(QDate date) const;
  int bucketCount() const;
  int bucketOf(const QDateTime &moment) const;
  QDate bucketStart(int index) const;

  friend bool operator==(const StatisticsRange &, const StatisticsRange &) = default;
};

} // namespace readary::services

#endif // READARY_SERVICES_STATISTICSRANGE_HPP
