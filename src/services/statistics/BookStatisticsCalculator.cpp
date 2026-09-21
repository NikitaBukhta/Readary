#include "services/statistics/BookStatisticsCalculator.hpp"

#include <algorithm>
#include <array>
#include <cstddef>

namespace {

constexpr double g_secondsPerHour = 3600.0;

// db/init.sql lets a finished row carry pages_to NULL, which ReadingSessionDTO
// reads back as 0 — a missing measurement, not zero pages. The schema's CHECK
// keeps pages_to >= pages_from on every other row, so this comparison
// identifies exactly that case.
bool isMissingEndPage(const readary::services::ReadingSessionDTO &session) {
  return session.pagesTo < session.pagesFrom;
}

} // namespace

namespace readary::services {

BookStatisticsDTO BookStatisticsCalculator::compute(const QList<ReadingSessionDTO> &sessions, QDate weekReference) {
  BookStatisticsDTO stats;
  stats.sessionCount = static_cast<int>(sessions.size());

  // Collected in a std::array so the day index goes through a bounds-checked
  // at(): QList::at() is read-only and the analysis gate refuses operator[].
  std::array<int, static_cast<std::size_t>(BookStatisticsDTO::kDaysInWeek)> weekBuckets{};

  // QDate::dayOfWeek() is 1 = Monday .. 7 = Sunday, so the week the reference
  // falls in starts that many days back and the same offset indexes the bucket.
  const QDate weekStart = weekReference.isValid() ? weekReference.addDays(1 - weekReference.dayOfWeek()) : QDate{};

  // All three speeds are derived from the same set — the sessions that were
  // actually timed. The average is then the duration-weighted mean of the
  // per-session speeds, so it can never fall outside min..max, which is what
  // the card prints it between. Journal totals stay whole: a session that
  // measured no speed still contributes its pages and its seconds to them.
  int timedPages = 0;
  int timedSeconds = 0;

  for (const ReadingSessionDTO &session : sessions) {
    const int pages = session.pagesRead();
    const int seconds = session.durationSeconds();
    stats.pagesRead += pages;
    stats.totalSeconds += seconds;

    if (weekStart.isValid() && session.startedAt.isValid()) {
      const qint64 dayIndex = weekStart.daysTo(session.startedAt.date());
      if (dayIndex >= 0 && dayIndex < BookStatisticsDTO::kDaysInWeek) {
        weekBuckets.at(static_cast<std::size_t>(dayIndex)) += pages;
      }
    }

    // No duration to divide by, or no end page to measure against — see
    // BookStatisticsDTO::timedSessionCount. A timed session that gained no
    // page is deliberately kept: 0 p/h is a real, slow session.
    if (seconds <= 0 || isMissingEndPage(session)) {
      continue;
    }
    timedPages += pages;
    timedSeconds += seconds;

    const double pagesPerHour = pages / (seconds / g_secondsPerHour);
    stats.minPagesPerHour = stats.timedSessionCount > 0 ? std::min(stats.minPagesPerHour, pagesPerHour) : pagesPerHour;
    stats.maxPagesPerHour = std::max(stats.maxPagesPerHour, pagesPerHour);
    ++stats.timedSessionCount;
  }
  stats.weeklyPages = QList<int>(weekBuckets.cbegin(), weekBuckets.cend());

  if (timedSeconds > 0) {
    stats.averagePagesPerHour = timedPages / (timedSeconds / g_secondsPerHour);
  }

  QList<ReadingSessionDTO> chronological = sessions;
  // BookTable orders the journal newest first; the curve runs the other way.
  // The row id breaks a tie, because two sessions can share a whole-second
  // stamp and only insertion order says which came first.
  std::ranges::sort(chronological, [](const ReadingSessionDTO &lhs, const ReadingSessionDTO &rhs) {
    return lhs.startedAt == rhs.startedAt ? lhs.id < rhs.id : lhs.startedAt < rhs.startedAt;
  });

  stats.progressPoints.reserve(chronological.size());
  for (qsizetype i = 0; i < chronological.size(); ++i) {
    const ReadingSessionDTO &session = chronological.at(i);
    // A row with no end page holds position rather than diving to the
    // baseline and back.
    const int page = isMissingEndPage(session) ? session.pagesFrom : session.pagesTo;
    stats.progressPoints.append({.session = static_cast<int>(i) + 1, .page = page});
  }

  return stats;
}

} // namespace readary::services
