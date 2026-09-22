#include "controllers/ProfileController.hpp"

#include "services/dto/BookStatus.hpp"
#include "services/storage/BookTable.hpp"
#include "support/TempLibrary.hpp"

#include <QSignalSpy>
#include <QTest>

#include <memory>

using Qt::StringLiterals::operator""_s;

using readary::controllers::ProfileController;
using readary::services::BookDTO;
using readary::services::BookStatus;
using readary::tests::makeBook;
using readary::tests::TempLibrary;

namespace {

constexpr qint64 kFinishedIsbn = 9780201616224LL;
constexpr qint64 kInProgressIsbn = 9781491903995LL;
constexpr qint64 kUntouchedIsbn = 9780134685991LL;

using ReaderLevel = ProfileController::ReaderLevel;

// makeBook gives every book 300 pages; the position is what varies here.
BookDTO bookAt(qint64 isbn, const QString &name, int status, int pagesRead) {
  BookDTO book = makeBook(isbn, name, status);
  book.pagesRead = pagesRead;
  return book;
}

} // namespace

class ProfileControllerTest : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void init();
  void cleanup();

  void newController_reportsAnEmptyLibrary();
  void refresh_readsTheWholeLibrary();
  void refresh_sumsTheJournalAcrossBooks();
  void refresh_staysQuietWhenNothingChanged();
  void refresh_picksUpABookAddedAfterwards();
  void refresh_followsABookFinishedAfterwards();
  void readerLevel_stepsWithTheFinishedCount_data();
  void readerLevel_stepsWithTheFinishedCount();

private:
  TempLibrary _library;
  std::unique_ptr<ProfileController> _controller;
};

void ProfileControllerTest::initTestCase() {
  // Resources of a static lib can be stripped by the linker — see main.cpp.
  Q_INIT_RESOURCE(db_scripts);
}

void ProfileControllerTest::init() {
  QVERIFY(_library.open());
  _controller = std::make_unique<ProfileController>(_library.books(), nullptr);
}

void ProfileControllerTest::cleanup() {
  _controller.reset();
  _library.close();
}

void ProfileControllerTest::newController_reportsAnEmptyLibrary() {
  // Nothing is read in the constructor — the page asks on open.
  QCOMPARE(_controller->statistics().booksTotal, 0);
  QVERIFY(!_controller->hasData());
  QCOMPARE(_controller->readerLevel(), ReaderLevel::Newcomer);
}

void ProfileControllerTest::refresh_readsTheWholeLibrary() {
  QCOMPARE(_library.books()->addBook(bookAt(kFinishedIsbn, u"Refactoring"_s, BookStatus::Finished, 300)),
           kFinishedIsbn);
  QCOMPARE(_library.books()->addBook(bookAt(kInProgressIsbn, u"Effective Modern C++"_s, BookStatus::InProgress, 120)),
           kInProgressIsbn);
  QCOMPARE(_library.books()->addBook(bookAt(kUntouchedIsbn, u"Clean Architecture"_s, BookStatus::WantToRead, 0)),
           kUntouchedIsbn);

  _controller->refresh();

  const auto stats = _controller->statistics();
  QCOMPARE(stats.booksTotal, 3);
  QCOMPARE(stats.booksFinished, 1);
  QCOMPARE(stats.booksInProgress, 1);
  QCOMPARE(stats.pagesRead, 420);
  QVERIFY(_controller->hasData());
}

void ProfileControllerTest::refresh_sumsTheJournalAcrossBooks() {
  QCOMPARE(_library.books()->addBook(bookAt(kFinishedIsbn, u"Refactoring"_s, BookStatus::Finished, 300)),
           kFinishedIsbn);
  QCOMPARE(_library.books()->addBook(bookAt(kInProgressIsbn, u"Effective Modern C++"_s, BookStatus::InProgress, 90)),
           kInProgressIsbn);
  QVERIFY(_library.books()->insertReadingSession(kFinishedIsbn, 0, 150, 40 * 60) > 0);
  QVERIFY(_library.books()->insertReadingSession(kFinishedIsbn, 150, 300, 50 * 60) > 0);
  QVERIFY(_library.books()->insertReadingSession(kInProgressIsbn, 0, 90, 30 * 60) > 0);

  _controller->refresh();

  const auto stats = _controller->statistics();
  QCOMPARE(stats.sessionCount, 3);
  QCOMPARE(stats.totalSeconds, 120 * 60);
  // The journal is not what pages are counted from — the reading positions are.
  QCOMPARE(stats.pagesRead, 390);
}

void ProfileControllerTest::refresh_staysQuietWhenNothingChanged() {
  QCOMPARE(_library.books()->addBook(bookAt(kFinishedIsbn, u"Refactoring"_s, BookStatus::Finished, 300)),
           kFinishedIsbn);
  _controller->refresh();

  QSignalSpy spy{_controller.get(), &ProfileController::statisticsChanged};
  _controller->refresh();

  QCOMPARE(spy.count(), 0);
}

void ProfileControllerTest::refresh_picksUpABookAddedAfterwards() {
  _controller->refresh();
  QSignalSpy spy{_controller.get(), &ProfileController::statisticsChanged};

  QCOMPARE(_library.books()->addBook(bookAt(kInProgressIsbn, u"Effective Modern C++"_s, BookStatus::InProgress, 40)),
           kInProgressIsbn);
  _controller->refresh();

  QCOMPARE(spy.count(), 1);
  QCOMPARE(_controller->statistics().booksTotal, 1);
  QCOMPARE(_controller->statistics().pagesRead, 40);
}

void ProfileControllerTest::refresh_followsABookFinishedAfterwards() {
  BookDTO book = bookAt(kInProgressIsbn, u"Effective Modern C++"_s, BookStatus::InProgress, 120);
  QCOMPARE(_library.books()->addBook(book), kInProgressIsbn);
  _controller->refresh();
  QCOMPARE(_controller->statistics().pagesRead, 120);

  book.status = BookStatus::Finished;
  QVERIFY(_library.books()->updateBook(book));
  _controller->refresh();

  QCOMPARE(_controller->statistics().booksFinished, 1);
  // Finished counts the book whole even though the position stayed at 120.
  QCOMPARE(_controller->statistics().pagesRead, 300);
  QCOMPARE(_controller->readerLevel(), ReaderLevel::Reader);
}

void ProfileControllerTest::readerLevel_stepsWithTheFinishedCount_data() {
  QTest::addColumn<int>("finished");
  QTest::addColumn<ReaderLevel>("expected");

  QTest::newRow("nothing finished") << 0 << ReaderLevel::Newcomer;
  QTest::newRow("first book") << 1 << ReaderLevel::Reader;
  QTest::newRow("just below bookworm") << 4 << ReaderLevel::Reader;
  QTest::newRow("bookworm") << 5 << ReaderLevel::Bookworm;
  QTest::newRow("just below bibliophile") << 19 << ReaderLevel::Bookworm;
  QTest::newRow("bibliophile") << 20 << ReaderLevel::Bibliophile;
}

void ProfileControllerTest::readerLevel_stepsWithTheFinishedCount() {
  QFETCH(int, finished);
  QFETCH(ReaderLevel, expected);

  // Local keys start at 1 and stay well below the ISBN-13 range, so books with
  // no ISBN are the cheapest way to fill a shelf of a given size.
  for (int i = 0; i < finished; ++i) {
    BookDTO book = makeBook(0, u"Finished %1"_s.arg(i), BookStatus::Finished);
    QVERIFY(_library.books()->addBook(book) > 0);
  }

  _controller->refresh();

  QCOMPARE(_controller->statistics().booksFinished, finished);
  QCOMPARE(_controller->readerLevel(), expected);
}

QTEST_GUILESS_MAIN(ProfileControllerTest)
#include "ProfileControllerTest.moc"
