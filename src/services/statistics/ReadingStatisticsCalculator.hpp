#ifndef READARY_SERVICES_READINGSTATISTICSCALCULATOR_HPP
#define READARY_SERVICES_READINGSTATISTICSCALCULATOR_HPP

#include "services/dto/BookDTO.hpp"
#include "services/dto/ReadingSessionDTO.hpp"
#include "services/dto/ReadingStatisticsDTO.hpp"

#include <QDate>
#include <QList>

namespace readary::services {

class ReadingStatisticsCalculator {
public:
  // `books` is the whole shelf, `sessions` the whole journal, in any order.
  // `today` anchors both the weekly buckets and the last month of the monthly
  // series; callers pass QDate::currentDate(), tests pass a fixed date.
  static ReadingStatisticsDTO compute(const QList<BookDTO> &books, const QList<ReadingSessionDTO> &sessions,
                                      QDate today);
};

} // namespace readary::services

#endif // READARY_SERVICES_READINGSTATISTICSCALCULATOR_HPP
