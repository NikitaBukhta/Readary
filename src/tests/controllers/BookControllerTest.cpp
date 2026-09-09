#include "controllers/BookController.hpp"
#include "models/books/BookListModel.hpp"
#include "models/books/BookSortFilterProxyModel.hpp"
#include "services/BookFilterCriteria.hpp"
#include "services/BookStatus.hpp"
#include "support/TempLibrary.hpp"

#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

#include <memory>

using Qt::StringLiterals::operator""_s;

using readary::controllers::BookController;
using readary::models::BookListModel;
using readary::services::BookDTO;
using readary::services::BookFilterCriteria;
using readary::services::BookStatus;
using readary::tests::makeBook;
using readary::tests::TempLibrary;

namespace {

constexpr qint64 kIsbn = 9780201616224LL;
constexpr qint64 kOtherIsbn = 9781491903995LL;
constexpr qint64 kUnknownIsbn = 9780000000000LL;

using ListKind = BookController::ListKind;

} // namespace

class BookControllerTest : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void init();
  void cleanup();

  void construction_buildsOneProxyPerCategory();
  void activeKind_defaultsToWantToRead();
  void setActiveKind_swapsTheSearchSource();
  void setActiveKind_toTheSameKind_isANoOp();

  void openBook_selectsAndAnnouncesIt();
  void openBook_withoutAnIsbn_isIgnored();
  void setCurrentBookIsbn_toTheSameBook_isANoOp();
  void currentBookData_carriesTheStoredBook();
  void currentBookData_carriesGenres();
  void currentBookData_withoutASelection_isEmpty();
  void currentBookData_forAnUnknownBook_isEmpty();

  void charactersModel_followsTheSelectedBook();
  void readingHistoryModel_followsTheSelectedBook();

  void setBookStatus_writesItThrough();
  void setBookStatus_toTheSameStatus_isANoOp();
  void setBookStatus_withoutASelection_isIgnored();
  void toggleWantToRead_setsAndClearsTheStatus();
  void toggleWishList_flipsTheFlag();
  void moveInProgressToWantToRead_parksTheProgress();
  void moveInProgressToWantToRead_onAnotherStatus_isIgnored();

  void hasCachedProgress_reportsTheParkedProgress();
  void restoreCachedProgress_putsThePagesBack();
  void restoreCachedProgress_withoutACache_isIgnored();
  void discardCachedProgress_dropsIt();

  void updateReadingProgress_movesThePosition();
  void updateReadingProgress_logsTheSession();
  void updateReadingProgress_toTheLastPage_finishesTheBook();
  void updateReadingProgress_backwards_isIgnored();
  void updateReadingProgress_pastTheLastPage_isIgnored();
  void updateReadingProgress_withoutADuration_logsNoSession();

  void deleteReadingSession_dropsTheEntry();
  void deleteReadingSession_invalidId_isIgnored();

  void importAndOpenBook_addsAndSelectsIt();
  void importAndOpenBook_alreadyKnownBook_justSelectsIt();
  void importAndOpenBook_withoutAnIsbn_isIgnored();

  void setFilterCriteria_narrowsTheSearchModel();

  void errorMessage_startsEmpty();
  void setBookStatus_whenTheWriteFails_reportsIt();
  void toggleWishList_whenTheWriteFails_reportsIt();
  void updateReadingProgress_whenTheWriteFails_reportsIt();
  void restoreCachedProgress_whenTheWriteFails_reportsIt();
  void errorMessage_onlyChangesWhenTheTextDoes();

private:
  void dropTheBookBehindTheControllersBack();

  TempLibrary _library;
  std::unique_ptr<BookListModel> _listModel;
  std::unique_ptr<BookController> _controller;
};

void BookControllerTest::initTestCase() {
  // Resources of a static lib can be stripped by the linker — see main.cpp.
  Q_INIT_RESOURCE(db_scripts);

  QStandardPaths::setTestModeEnabled(true);
  QCoreApplication::setOrganizationName("DarieszzBooksTests");
  QCoreApplication::setOrganizationDomain("tests.darieszzbooks.local");
  QCoreApplication::setApplicationName("BookControllerTest");
}

void BookControllerTest::init() {
  QSettings settings;
  settings.clear();
  settings.sync();

  QVERIFY(_library.open());

  BookDTO refactoring = makeBook(kIsbn, u"Refactoring"_s, BookStatus::InProgress);
  refactoring.totalPages = 448;
  refactoring.pagesRead = 100;
  refactoring.genres = {u"Software"_s, u"Craft"_s};
  QCOMPARE(_library.books()->addBook(refactoring), kIsbn);

  BookDTO effective = makeBook(kOtherIsbn, u"Effective Modern C++"_s, BookStatus::WantToRead);
  QCOMPARE(_library.books()->addBook(effective), kOtherIsbn);

  _listModel = std::make_unique<BookListModel>(_library.books(), nullptr);
  _controller = std::make_unique<BookController>(_library.books(), _listModel.get(), nullptr);

  // AppInitializer owns this cross-domain wiring in the running app.
  connect(_controller.get(), &BookController::bookSaved, _listModel.get(), &BookListModel::refresh);
}

void BookControllerTest::cleanup() {
  _controller.reset();
  _listModel.reset();
  _library.close();
}

// Pulls the row out from under the controller while its list model still has
// the book cached, so the next write matches nothing — the only way to reach
// the failure branches without a broken database.
void BookControllerTest::dropTheBookBehindTheControllersBack() {
  QVERIFY(_library.db()->exec(u"DELETE FROM books WHERE isbn = 9780201616224"_s));
}

void BookControllerTest::construction_buildsOneProxyPerCategory() {
  QVERIFY(_controller->getSortFilterProxyForKind(ListKind::WantToRead) != nullptr);
  QVERIFY(_controller->getSortFilterProxyForKind(ListKind::WantToBuy) != nullptr);
  QVERIFY(_controller->getSortFilterProxyForKind(ListKind::AlreadyRead) != nullptr);
  QVERIFY(_controller->getSortFilterProxyForKind(ListKind::InProgress) != nullptr);

  // Each proxy carries its category's filter strategy.
  QCOMPARE(_controller->getSortFilterProxyForKind(ListKind::InProgress)->rowCount(), 1);
  QCOMPARE(_controller->getSortFilterProxyForKind(ListKind::WantToRead)->rowCount(), 1);
  QCOMPARE(_controller->getSortFilterProxyForKind(ListKind::WantToBuy)->rowCount(), 0);
  QCOMPARE(_controller->getSortFilterProxyForKind(ListKind::AlreadyRead)->rowCount(), 0);
}

void BookControllerTest::activeKind_defaultsToWantToRead() {
  QCOMPARE(_controller->activeKind(), ListKind::WantToRead);
  QCOMPARE(_controller->searchModel()->rowCount(), 1);
}

void BookControllerTest::setActiveKind_swapsTheSearchSource() {
  QSignalSpy kindSpy{_controller.get(), &BookController::activeKindChanged};

  _controller->setActiveKind(ListKind::InProgress);

  QCOMPARE(kindSpy.count(), 1);
  QCOMPARE(_controller->activeKind(), ListKind::InProgress);
  QCOMPARE(_controller->searchModel()->rowCount(), 1);
  QCOMPARE(_controller->searchModel()->index(0, 0).data(BookListModel::NameRole).toString(), u"Refactoring"_s);
}

void BookControllerTest::setActiveKind_toTheSameKind_isANoOp() {
  QSignalSpy kindSpy{_controller.get(), &BookController::activeKindChanged};

  _controller->setActiveKind(ListKind::WantToRead);

  QCOMPARE(kindSpy.count(), 0);
}

void BookControllerTest::openBook_selectsAndAnnouncesIt() {
  QSignalSpy openSpy{_controller.get(), &BookController::bookOpenRequested};
  QSignalSpy isbnSpy{_controller.get(), &BookController::currentBookIsbnChanged};

  _controller->openBook(kIsbn);

  QCOMPARE(_controller->currentBookIsbn(), kIsbn);
  QCOMPARE(isbnSpy.count(), 1);
  QCOMPARE(openSpy.count(), 1);
  QCOMPARE(openSpy.first().first().toLongLong(), kIsbn);
}

void BookControllerTest::openBook_withoutAnIsbn_isIgnored() {
  QSignalSpy openSpy{_controller.get(), &BookController::bookOpenRequested};

  _controller->openBook(0);

  QCOMPARE(openSpy.count(), 0);
  QCOMPARE(_controller->currentBookIsbn(), 0LL);
}

void BookControllerTest::setCurrentBookIsbn_toTheSameBook_isANoOp() {
  _controller->setCurrentBookIsbn(kIsbn);

  QSignalSpy isbnSpy{_controller.get(), &BookController::currentBookIsbnChanged};
  _controller->setCurrentBookIsbn(kIsbn);

  QCOMPARE(isbnSpy.count(), 0);
}

void BookControllerTest::currentBookData_carriesTheStoredBook() {
  _controller->setCurrentBookIsbn(kIsbn);

  const auto book = _controller->currentBookData();
  QCOMPARE(book.isbn, kIsbn);
  QCOMPARE(book.name, u"Refactoring"_s);
  QCOMPARE(book.totalPages, 448);
  QCOMPARE(book.pagesRead, 100);
  QCOMPARE(book.status, static_cast<int>(BookStatus::InProgress));
}

void BookControllerTest::currentBookData_carriesGenres() {
  // Genres live in their own table, so the controller joins them in.
  _controller->setCurrentBookIsbn(kIsbn);

  QCOMPARE(_controller->currentBookData().genres, QStringList({u"Craft"_s, u"Software"_s}));
}

void BookControllerTest::currentBookData_withoutASelection_isEmpty() {
  QCOMPARE(_controller->currentBookData().isbn, 0LL);
}

void BookControllerTest::currentBookData_forAnUnknownBook_isEmpty() {
  _controller->setCurrentBookIsbn(kUnknownIsbn);

  QCOMPARE(_controller->currentBookData().isbn, 0LL);
}

void BookControllerTest::charactersModel_followsTheSelectedBook() {
  QVERIFY(_library.db()->exec(u"INSERT INTO book_characters (book_isbn, name) VALUES (9780201616224, 'Extract')"_s));

  _controller->setCurrentBookIsbn(kIsbn);
  QCOMPARE(_controller->charactersModel()->rowCount(), 1);

  _controller->setCurrentBookIsbn(kOtherIsbn);
  QCOMPARE(_controller->charactersModel()->rowCount(), 0);
}

void BookControllerTest::readingHistoryModel_followsTheSelectedBook() {
  QVERIFY(_library.books()->insertReadingSession(kIsbn, 0, 100, 600) > 0);

  _controller->setCurrentBookIsbn(kIsbn);
  QCOMPARE(_controller->readingHistoryModel()->totalCount(), 1);

  _controller->setCurrentBookIsbn(kOtherIsbn);
  QCOMPARE(_controller->readingHistoryModel()->totalCount(), 0);
}

void BookControllerTest::setBookStatus_writesItThrough() {
  _controller->setCurrentBookIsbn(kIsbn);
  QSignalSpy savedSpy{_controller.get(), &BookController::bookSaved};

  _controller->setBookStatus(BookStatus::Finished);

  QCOMPARE(savedSpy.count(), 1);
  QCOMPARE(_controller->currentBookData().status, static_cast<int>(BookStatus::Finished));
  QCOMPARE(_library.books()->getAllBooks().first().status, static_cast<int>(BookStatus::Finished));
}

void BookControllerTest::setBookStatus_toTheSameStatus_isANoOp() {
  _controller->setCurrentBookIsbn(kIsbn);
  QSignalSpy savedSpy{_controller.get(), &BookController::bookSaved};

  _controller->setBookStatus(BookStatus::InProgress);

  QCOMPARE(savedSpy.count(), 0);
}

void BookControllerTest::setBookStatus_withoutASelection_isIgnored() {
  QSignalSpy savedSpy{_controller.get(), &BookController::bookSaved};

  _controller->setBookStatus(BookStatus::Finished);

  QCOMPARE(savedSpy.count(), 0);
}

void BookControllerTest::toggleWantToRead_setsAndClearsTheStatus() {
  _controller->setCurrentBookIsbn(kOtherIsbn);
  QCOMPARE(_controller->currentBookData().status, static_cast<int>(BookStatus::WantToRead));

  _controller->toggleWantToRead();
  QCOMPARE(_controller->currentBookData().status, static_cast<int>(BookStatus::None));

  _controller->toggleWantToRead();
  QCOMPARE(_controller->currentBookData().status, static_cast<int>(BookStatus::WantToRead));
}

void BookControllerTest::toggleWishList_flipsTheFlag() {
  _controller->setCurrentBookIsbn(kIsbn);
  QVERIFY(!_controller->currentBookData().inWishList);

  _controller->toggleWishList();
  QVERIFY(_controller->currentBookData().inWishList);

  _controller->toggleWishList();
  QVERIFY(!_controller->currentBookData().inWishList);
}

void BookControllerTest::moveInProgressToWantToRead_parksTheProgress() {
  _controller->setCurrentBookIsbn(kIsbn);

  _controller->moveInProgressToWantToRead();

  const auto book = _controller->currentBookData();
  QCOMPARE(book.status, static_cast<int>(BookStatus::WantToRead));
  QCOMPARE(book.pagesRead, 0);
  // The pages are not lost, only parked until the reader comes back.
  QVERIFY(_controller->hasCachedProgress());
}

void BookControllerTest::moveInProgressToWantToRead_onAnotherStatus_isIgnored() {
  _controller->setCurrentBookIsbn(kOtherIsbn);
  QSignalSpy savedSpy{_controller.get(), &BookController::bookSaved};

  _controller->moveInProgressToWantToRead();

  QCOMPARE(savedSpy.count(), 0);
  QCOMPARE(_controller->currentBookData().status, static_cast<int>(BookStatus::WantToRead));
}

void BookControllerTest::hasCachedProgress_reportsTheParkedProgress() {
  _controller->setCurrentBookIsbn(kIsbn);
  QVERIFY(!_controller->hasCachedProgress());

  _controller->moveInProgressToWantToRead();

  QVERIFY(_controller->hasCachedProgress());
}

void BookControllerTest::restoreCachedProgress_putsThePagesBack() {
  _controller->setCurrentBookIsbn(kIsbn);
  _controller->moveInProgressToWantToRead();

  _controller->restoreCachedProgress();

  const auto book = _controller->currentBookData();
  QCOMPARE(book.pagesRead, 100);
  QCOMPARE(book.status, static_cast<int>(BookStatus::InProgress));
  // Restoring consumes the cache.
  QVERIFY(!_controller->hasCachedProgress());
}

void BookControllerTest::restoreCachedProgress_withoutACache_isIgnored() {
  _controller->setCurrentBookIsbn(kIsbn);
  QSignalSpy savedSpy{_controller.get(), &BookController::bookSaved};

  _controller->restoreCachedProgress();

  QCOMPARE(savedSpy.count(), 0);
}

void BookControllerTest::discardCachedProgress_dropsIt() {
  _controller->setCurrentBookIsbn(kIsbn);
  _controller->moveInProgressToWantToRead();
  QVERIFY(_controller->hasCachedProgress());

  _controller->discardCachedProgress();

  QVERIFY(!_controller->hasCachedProgress());
}

void BookControllerTest::updateReadingProgress_movesThePosition() {
  _controller->setCurrentBookIsbn(kIsbn);
  QSignalSpy savedSpy{_controller.get(), &BookController::bookSaved};

  _controller->updateReadingProgress(200, 600);

  QCOMPARE(savedSpy.count(), 1);
  QCOMPARE(_controller->currentBookData().pagesRead, 200);
  QCOMPARE(_controller->currentBookData().status, static_cast<int>(BookStatus::InProgress));
}

void BookControllerTest::updateReadingProgress_logsTheSession() {
  _controller->setCurrentBookIsbn(kIsbn);

  _controller->updateReadingProgress(200, 600);

  const auto sessions = _library.books()->getReadingSessions(kIsbn);
  QCOMPARE(sessions.size(), 1);
  QCOMPARE(sessions.first().pagesFrom, 100);
  QCOMPARE(sessions.first().pagesTo, 200);
  QCOMPARE(sessions.first().durationSeconds(), 600);
}

void BookControllerTest::updateReadingProgress_toTheLastPage_finishesTheBook() {
  _controller->setCurrentBookIsbn(kIsbn);

  _controller->updateReadingProgress(448, 600);

  QCOMPARE(_controller->currentBookData().status, static_cast<int>(BookStatus::Finished));
}

void BookControllerTest::updateReadingProgress_backwards_isIgnored() {
  _controller->setCurrentBookIsbn(kIsbn);
  QSignalSpy savedSpy{_controller.get(), &BookController::bookSaved};

  _controller->updateReadingProgress(50, 600);

  QCOMPARE(savedSpy.count(), 0);
  QCOMPARE(_controller->currentBookData().pagesRead, 100);
  QVERIFY(_library.books()->getReadingSessions(kIsbn).isEmpty());
}

void BookControllerTest::updateReadingProgress_pastTheLastPage_isIgnored() {
  _controller->setCurrentBookIsbn(kIsbn);
  QSignalSpy savedSpy{_controller.get(), &BookController::bookSaved};

  _controller->updateReadingProgress(999, 600);

  QCOMPARE(savedSpy.count(), 0);
  QCOMPARE(_controller->currentBookData().pagesRead, 100);
}

void BookControllerTest::updateReadingProgress_withoutADuration_logsNoSession() {
  // Typing a page number by hand moves the position without inventing a session.
  _controller->setCurrentBookIsbn(kIsbn);

  _controller->updateReadingProgress(200, 0);

  QCOMPARE(_controller->currentBookData().pagesRead, 200);
  QVERIFY(_library.books()->getReadingSessions(kIsbn).isEmpty());
}

void BookControllerTest::deleteReadingSession_dropsTheEntry() {
  const qint64 id = _library.books()->insertReadingSession(kIsbn, 0, 100, 600);
  QVERIFY(id > 0);
  _controller->setCurrentBookIsbn(kIsbn);
  QCOMPARE(_controller->readingHistoryModel()->totalCount(), 1);

  _controller->deleteReadingSession(QString::number(id));

  QCOMPARE(_controller->readingHistoryModel()->totalCount(), 0);
  QVERIFY(_library.books()->getReadingSessions(kIsbn).isEmpty());
}

void BookControllerTest::deleteReadingSession_invalidId_isIgnored() {
  const qint64 id = _library.books()->insertReadingSession(kIsbn, 0, 100, 600);
  QVERIFY(id > 0);
  _controller->setCurrentBookIsbn(kIsbn);

  _controller->deleteReadingSession(u"not-a-number"_s);
  _controller->deleteReadingSession(QString{});
  _controller->deleteReadingSession(u"0"_s);

  QCOMPARE(_library.books()->getReadingSessions(kIsbn).size(), 1);
}

void BookControllerTest::importAndOpenBook_addsAndSelectsIt() {
  QSignalSpy savedSpy{_controller.get(), &BookController::bookSaved};
  QSignalSpy openSpy{_controller.get(), &BookController::bookOpenRequested};

  _controller->importAndOpenBook(makeBook(kUnknownIsbn, u"Dune"_s, BookStatus::WantToRead));

  QCOMPARE(savedSpy.count(), 1);
  QCOMPARE(openSpy.count(), 1);
  QCOMPARE(_controller->currentBookIsbn(), kUnknownIsbn);
  QCOMPARE(_controller->currentBookData().name, u"Dune"_s);
}

void BookControllerTest::importAndOpenBook_alreadyKnownBook_justSelectsIt() {
  QSignalSpy savedSpy{_controller.get(), &BookController::bookSaved};

  _controller->importAndOpenBook(makeBook(kIsbn, u"Refactoring"_s));

  QCOMPARE(savedSpy.count(), 0);
  QCOMPARE(_controller->currentBookIsbn(), kIsbn);
  // The stored copy wins — importing must not overwrite the reader's own data.
  QCOMPARE(_controller->currentBookData().pagesRead, 100);
}

void BookControllerTest::importAndOpenBook_withoutAnIsbn_isIgnored() {
  QSignalSpy openSpy{_controller.get(), &BookController::bookOpenRequested};

  _controller->importAndOpenBook(makeBook(0, u"Nameless"_s));

  QCOMPARE(openSpy.count(), 0);
  QCOMPARE(_controller->currentBookIsbn(), 0LL);
}

void BookControllerTest::setFilterCriteria_narrowsTheSearchModel() {
  _controller->setActiveKind(ListKind::InProgress);
  QCOMPARE(_controller->searchModel()->rowCount(), 1);

  BookFilterCriteria criteria;
  criteria.author = u"Nobody At All"_s;
  _controller->setFilterCriteria(criteria);

  QCOMPARE(_controller->searchModel()->rowCount(), 0);
}

void BookControllerTest::errorMessage_startsEmpty() { QVERIFY(_controller->errorMessage().isEmpty()); }

void BookControllerTest::setBookStatus_whenTheWriteFails_reportsIt() {
  _controller->setCurrentBookIsbn(kIsbn);
  dropTheBookBehindTheControllersBack();

  QSignalSpy errorSpy{_controller.get(), &BookController::errorMessageChanged};
  QSignalSpy savedSpy{_controller.get(), &BookController::bookSaved};
  _controller->setBookStatus(BookStatus::Finished);

  QCOMPARE(errorSpy.count(), 1);
  QVERIFY(!_controller->errorMessage().isEmpty());
  // A failed write must not announce a save.
  QCOMPARE(savedSpy.count(), 0);
}

void BookControllerTest::toggleWishList_whenTheWriteFails_reportsIt() {
  _controller->setCurrentBookIsbn(kIsbn);
  dropTheBookBehindTheControllersBack();

  QSignalSpy savedSpy{_controller.get(), &BookController::bookSaved};
  _controller->toggleWishList();

  QVERIFY(!_controller->errorMessage().isEmpty());
  QCOMPARE(savedSpy.count(), 0);
}

void BookControllerTest::updateReadingProgress_whenTheWriteFails_reportsIt() {
  _controller->setCurrentBookIsbn(kIsbn);
  dropTheBookBehindTheControllersBack();

  QSignalSpy savedSpy{_controller.get(), &BookController::bookSaved};
  _controller->updateReadingProgress(200, 600);

  QVERIFY(!_controller->errorMessage().isEmpty());
  QCOMPARE(savedSpy.count(), 0);
  // The session log is only written after the position write succeeds.
  QVERIFY(_library.books()->getReadingSessions(kIsbn).isEmpty());
}

void BookControllerTest::restoreCachedProgress_whenTheWriteFails_reportsIt() {
  _controller->setCurrentBookIsbn(kIsbn);
  _controller->moveInProgressToWantToRead();
  QVERIFY(_controller->hasCachedProgress());
  dropTheBookBehindTheControllersBack();

  _controller->restoreCachedProgress();

  QVERIFY(!_controller->errorMessage().isEmpty());
}

void BookControllerTest::errorMessage_onlyChangesWhenTheTextDoes() {
  _controller->setCurrentBookIsbn(kIsbn);
  dropTheBookBehindTheControllersBack();

  QSignalSpy errorSpy{_controller.get(), &BookController::errorMessageChanged};
  // The failed write leaves the cached status untouched, so the second call
  // takes the same branch and produces the same text.
  _controller->setBookStatus(BookStatus::Finished);
  _controller->setBookStatus(BookStatus::Finished);

  QCOMPARE(errorSpy.count(), 1);
}

QTEST_GUILESS_MAIN(BookControllerTest)
#include "BookControllerTest.moc"
