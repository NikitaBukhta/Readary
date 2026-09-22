#include "services/statistics/LibraryStatisticsCalculator.hpp"

#include "services/dto/BookStatus.hpp"

#include <QTest>

using Qt::StringLiterals::operator""_s;

using readary::services::BookDTO;
using readary::services::BookStatus;
using readary::services::LibraryStatisticsCalculator;
using readary::services::LibraryStatisticsDTO;
using readary::services::ReadingSessionDTO;

namespace {

BookDTO book(int status, int pagesRead, int totalPages) {
  BookDTO dto;
  dto.name = u"Book"_s;
  dto.status = status;
  dto.pagesRead = pagesRead;
  dto.totalPages = totalPages;
  return dto;
}

ReadingSessionDTO session(int seconds, int pagesFrom = 0, int pagesTo = 0) {
  ReadingSessionDTO dto;
  dto.endedAt = QDateTime::currentDateTimeUtc();
  dto.startedAt = dto.endedAt.addSecs(-seconds);
  dto.pagesFrom = pagesFrom;
  dto.pagesTo = pagesTo;
  return dto;
}

} // namespace

class LibraryStatisticsCalculatorTest : public QObject {
  Q_OBJECT

private slots:
  void emptyLibrary_isAllZeroes();
  void countsTheShelfByStatus();
  void pagesRead_sumsTheReadingPositions();
  void pagesRead_countsAFinishedBookWhole();
  void pagesRead_keepsAPositionPastTheStatedLength();
  void journal_contributesTimeButNotPages();
  void twoLibrariesWithTheSameFigures_compareEqual();
};

void LibraryStatisticsCalculatorTest::emptyLibrary_isAllZeroes() {
  const LibraryStatisticsDTO stats = LibraryStatisticsCalculator::compute({}, {});

  QCOMPARE(stats.booksTotal, 0);
  QCOMPARE(stats.booksFinished, 0);
  QCOMPARE(stats.booksInProgress, 0);
  QCOMPARE(stats.pagesRead, 0);
  QCOMPARE(stats.totalSeconds, 0);
  QCOMPARE(stats.sessionCount, 0);
}

void LibraryStatisticsCalculatorTest::countsTheShelfByStatus() {
  const QList<BookDTO> books{
      book(BookStatus::Finished, 300, 300), book(BookStatus::Finished, 200, 200), book(BookStatus::InProgress, 50, 400),
      book(BookStatus::WantToRead, 0, 350), book(BookStatus::None, 0, 120),
  };

  const LibraryStatisticsDTO stats = LibraryStatisticsCalculator::compute(books, {});

  QCOMPARE(stats.booksTotal, 5);
  QCOMPARE(stats.booksFinished, 2);
  QCOMPARE(stats.booksInProgress, 1);
}

void LibraryStatisticsCalculatorTest::pagesRead_sumsTheReadingPositions() {
  const QList<BookDTO> books{
      book(BookStatus::InProgress, 120, 400),
      book(BookStatus::InProgress, 35, 200),
      book(BookStatus::WantToRead, 0, 350),
  };

  QCOMPARE(LibraryStatisticsCalculator::compute(books, {}).pagesRead, 155);
}

void LibraryStatisticsCalculatorTest::pagesRead_countsAFinishedBookWhole() {
  // Status set by hand from the detail page never touches pagesRead, so the
  // position stays at 0 while the book is in fact behind the reader.
  const QList<BookDTO> books{book(BookStatus::Finished, 0, 300)};

  QCOMPARE(LibraryStatisticsCalculator::compute(books, {}).pagesRead, 300);
}

void LibraryStatisticsCalculatorTest::pagesRead_keepsAPositionPastTheStatedLength() {
  // An imported page count can be lower than what was actually read; the
  // position is the measurement, so it wins.
  const QList<BookDTO> books{book(BookStatus::Finished, 420, 300)};

  QCOMPARE(LibraryStatisticsCalculator::compute(books, {}).pagesRead, 420);
}

void LibraryStatisticsCalculatorTest::journal_contributesTimeButNotPages() {
  // The journal records re-reads too, so its pages would double-count against
  // the reading positions — only its duration is taken.
  const QList<BookDTO> books{book(BookStatus::Finished, 300, 300)};
  const QList<ReadingSessionDTO> sessions{session(30 * 60, 0, 150), session(45 * 60, 150, 300)};

  const LibraryStatisticsDTO stats = LibraryStatisticsCalculator::compute(books, sessions);

  QCOMPARE(stats.sessionCount, 2);
  QCOMPARE(stats.totalSeconds, 75 * 60);
  QCOMPARE(stats.pagesRead, 300);
}

void LibraryStatisticsCalculatorTest::twoLibrariesWithTheSameFigures_compareEqual() {
  // The controller drops a recompute that changed nothing, which rests on this.
  const QList<BookDTO> books{book(BookStatus::InProgress, 10, 100)};

  QCOMPARE(LibraryStatisticsCalculator::compute(books, {}), LibraryStatisticsCalculator::compute(books, {}));
}

QTEST_GUILESS_MAIN(LibraryStatisticsCalculatorTest)
#include "LibraryStatisticsCalculatorTest.moc"
