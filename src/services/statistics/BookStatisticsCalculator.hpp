#ifndef READARY_SERVICES_BOOKSTATISTICSCALCULATOR_HPP
#define READARY_SERVICES_BOOKSTATISTICSCALCULATOR_HPP

#include "services/dto/BookStatisticsDTO.hpp"
#include "services/dto/JournalFiguresDTO.hpp"
#include "services/dto/ReadingSessionDTO.hpp"

#include <QDate>
#include <QList>

namespace readary::services {

class BookStatisticsCalculator {
public:
  static BookStatisticsDTO compute(const QList<ReadingSessionDTO> &sessions, QDate weekReference);
  static JournalFiguresDTO journalFigures(const QList<ReadingSessionDTO> &sessions);
};

} // namespace readary::services

#endif // READARY_SERVICES_BOOKSTATISTICSCALCULATOR_HPP
