#ifndef READARY_SERVICES_READINGSTATISTICSCALCULATOR_HPP
#define READARY_SERVICES_READINGSTATISTICSCALCULATOR_HPP

#include "services/dto/BookDTO.hpp"
#include "services/dto/ReadingSessionDTO.hpp"
#include "services/dto/ReadingStatisticsDTO.hpp"
#include "services/dto/StatisticsRange.hpp"

#include <QDate>

namespace readary::services {

class ReadingStatisticsCalculator {
public:
  static ReadingStatisticsDTO compute(const QList<BookDTO> &books, const QList<ReadingSessionDTO> &sessions,
                                      const StatisticsRange &range);
  static QDate earliestSession(const QList<ReadingSessionDTO> &sessions);
};

} // namespace readary::services

#endif // READARY_SERVICES_READINGSTATISTICSCALCULATOR_HPP
