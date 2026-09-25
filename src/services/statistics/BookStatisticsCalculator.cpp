#include "services/statistics/BookStatisticsCalculator.hpp"

#include "services/statistics/StatisticsPeriods.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iterator>
#include <vector>

namespace {

double hoursIn(int seconds) {
  return std::chrono::duration<double, std::chrono::hours::period>{std::chrono::seconds{seconds}}.count();
}

} // namespace

namespace readary::services {

JournalFiguresDTO BookStatisticsCalculator::journalFigures(const QList<ReadingSessionDTO> &sessions) {
  JournalFiguresDTO figures;
  figures.sessionCount = static_cast<int>(sessions.size());

  int timedPages = 0;
  int timedSeconds = 0;

  for (const ReadingSessionDTO &session : sessions) {
    const int pages = session.pagesRead();
    const int seconds = session.durationSeconds();
    figures.pagesRead += pages;
    figures.totalSeconds += seconds;

    if (seconds <= 0 || !session.hasEndPage()) {
      continue;
    }
    timedPages += pages;
    timedSeconds += seconds;

    const double pagesPerHour = pages / hoursIn(seconds);
    figures.minPagesPerHour =
        figures.timedSessionCount > 0 ? std::min(figures.minPagesPerHour, pagesPerHour) : pagesPerHour;
    figures.maxPagesPerHour = std::max(figures.maxPagesPerHour, pagesPerHour);
    ++figures.timedSessionCount;
  }

  if (timedSeconds > 0) {
    figures.averagePagesPerHour = timedPages / hoursIn(timedSeconds);
  }
  return figures;
}

BookStatisticsDTO BookStatisticsCalculator::compute(const QList<ReadingSessionDTO> &sessions, QDate weekReference) {
  BookStatisticsDTO stats{journalFigures(sessions)};

  const StatisticsRange week = StatisticsPeriods::week(weekReference);
  std::vector<int> days(static_cast<std::size_t>(week.bucketCount()), 0);
  for (const ReadingSessionDTO &session : sessions) {
    if (const int day = week.bucketOf(session.startedAt); day >= 0) {
      days.at(static_cast<std::size_t>(day)) += session.pagesRead();
    }
  }
  stats.weeklyPages = QList<int>(std::make_move_iterator(days.begin()), std::make_move_iterator(days.end()));

  QList<ReadingSessionDTO> chronological = sessions;
  std::ranges::sort(chronological,
                    [](const ReadingSessionDTO &lhs, const ReadingSessionDTO &rhs) { return lhs.startedBefore(rhs); });

  stats.progressPoints.reserve(chronological.size());
  for (qsizetype i = 0; i < chronological.size(); ++i) {
    stats.progressPoints.append({.session = static_cast<int>(i) + 1, .page = chronological.at(i).endPage()});
  }

  return stats;
}

} // namespace readary::services
