#ifndef READARY_SERVICES_LIBRARYSTATISTICSCALCULATOR_HPP
#define READARY_SERVICES_LIBRARYSTATISTICSCALCULATOR_HPP

#include "services/dto/BookDTO.hpp"
#include "services/dto/LibraryStatisticsDTO.hpp"
#include "services/dto/ReadingSessionDTO.hpp"

#include <QList>

namespace readary::services {

class LibraryStatisticsCalculator {
public:
  // `books` is the whole shelf, `sessions` the whole journal — the two are
  // counted independently, so a library read without ever starting the timer
  // still reports its pages.
  static LibraryStatisticsDTO compute(const QList<BookDTO> &books, const QList<ReadingSessionDTO> &sessions);
};

} // namespace readary::services

#endif // READARY_SERVICES_LIBRARYSTATISTICSCALCULATOR_HPP
