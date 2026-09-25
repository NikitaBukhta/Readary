#ifndef READARY_SERVICES_JOURNALFIGURESDTO_HPP
#define READARY_SERVICES_JOURNALFIGURESDTO_HPP

namespace readary::services {

struct JournalFiguresDTO {
  int sessionCount = 0;
  int timedSessionCount = 0;
  int pagesRead = 0;
  int totalSeconds = 0;
  double averagePagesPerHour = 0.0;
  double minPagesPerHour = 0.0;
  double maxPagesPerHour = 0.0;

  friend bool operator==(const JournalFiguresDTO &, const JournalFiguresDTO &) = default;
};

} // namespace readary::services

#endif // READARY_SERVICES_JOURNALFIGURESDTO_HPP
