#ifndef READARY_SERVICES_LIBRARYSTATISTICSDTO_HPP
#define READARY_SERVICES_LIBRARYSTATISTICSDTO_HPP

namespace readary::services {

// Everything the profile page shows about the library as a whole, derived from
// the books table and the reading_sessions journal. Nothing here is stored.
struct LibraryStatisticsDTO {
  int booksTotal = 0;
  int booksFinished = 0;
  int booksInProgress = 0;
  // Counted per book rather than summed over the journal: the journal only
  // holds sessions that went through the reading timer, and a book marked
  // finished by hand has never been near it. Re-reads therefore do not count
  // twice here, unlike BookStatisticsDTO::pagesRead.
  int pagesRead = 0;
  // Journal figures — what the timer actually measured.
  int totalSeconds = 0;
  int sessionCount = 0;

  // Lets the controller drop a recompute that changed nothing instead of
  // telling QML to rebuild the page.
  friend bool operator==(const LibraryStatisticsDTO &, const LibraryStatisticsDTO &) = default;
};

} // namespace readary::services

#endif // READARY_SERVICES_LIBRARYSTATISTICSDTO_HPP
