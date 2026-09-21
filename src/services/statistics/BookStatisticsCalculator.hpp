#ifndef READARY_SERVICES_BOOKSTATISTICSCALCULATOR_HPP
#define READARY_SERVICES_BOOKSTATISTICSCALCULATOR_HPP

#include "services/dto/BookStatisticsDTO.hpp"
#include "services/dto/ReadingSessionDTO.hpp"

#include <QDate>
#include <QList>

namespace readary::services {

class BookStatisticsCalculator {
public:
  // `sessions` may arrive in any order — BookTable hands them back newest
  // first, the progress curve needs them oldest first. `weekReference` picks
  // the Monday..Sunday week the weekly buckets cover; callers pass
  // QDate::currentDate(), tests pass a fixed date.
  static BookStatisticsDTO compute(const QList<ReadingSessionDTO> &sessions, QDate weekReference);
};

} // namespace readary::services

#endif // READARY_SERVICES_BOOKSTATISTICSCALCULATOR_HPP
