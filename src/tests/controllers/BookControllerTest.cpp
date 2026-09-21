#include "controllers/BookController.hpp"
#include "models/books/list/BookListModel.hpp"
#include "models/books/proxy/BookSortFilterProxyModel.hpp"
#include "services/dto/BookStatus.hpp"
#include "services/filtering/BookFilterCriteria.hpp"
#include "services/pdf/PdfSource.hpp"
#include "support/MinimalPdf.hpp"
#include "support/TempLibrary.hpp"

#include <QDir>
#include <QFile>
#include <QImage>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

#include <memory>

using Qt::StringLiterals::operator""_s;

using readary::controllers::BookController;
using readary::models::BookListModel;
using readary::services::BookDTO;
using readary::services::BookFilterCriteria;
using readary::services::BookStatus;
using readary::services::PdfSource;
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
  void readingJournalChanged_firesOnEverySessionWrite();
  void readingJournalChanged_staysQuietWhenNothingWasWritten();

  void importAndOpenBook_addsAndSelectsIt();
  void importAndOpenBook_alreadyKnownBook_justSelectsIt();
  void importAndOpenBook_withoutAnIsbn_isIgnored();

  void addCustomBook_storesTheFormAndOpensIt();
  void addCustomBook_withoutATitle_isRejected();
  void addCustomBook_withoutAnAuthor_isRejected();
  void addCustomBook_trimsTheText();
  void addCustomBook_withoutAnIsbn_doesNotInventOne();
  void addCustomBook_keysEachBookSeparately();
  void addCustomBook_storesTheTypedIsbn();
  void addCustomBook_normalisesATypedIsbn10();
  void addCustomBook_invalidTypedIsbn_isRefused();
  void addCustomBook_pdfIsbnOverridesTheTypedOne();
  void addCustomBook_anIsbnAlreadyInTheLibrary_isRefused();

  void deleteCurrentBook_removesACustomBook();
  void deleteCurrentBook_aCatalogBook_isRefused();
  void deleteCurrentBook_withoutASelection_isIgnored();
  void deleteCurrentBook_dropsTheParkedProgress();
  void deleteCurrentBook_dropsItsFiles();

  void stagePdf_reportsWhatThePdfSays();
  void stagePdf_aFileThatIsNotAPdf_isRejected();
  void stagePdf_handsBackTheCoverInline();
  void clearStagedPdf_dropsIt();
  void addCustomBook_withAStagedPdf_storesItAndTheCover();
  void addCustomBook_copiesAPickedCoverIntoTheStore();
  void addCustomBook_pickedCoverSurvivesTheOriginalBeingDeleted();
  void addCustomBook_pdfPageCountOverridesTheForm();
  void addCustomBook_pdfMetadataOverridesTheForm();
  void addCustomBook_pdfWithoutMetadata_keepsTheTypedText();
  void addCustomBook_pdfSuppliesTheTitleTheFormLeftBlank();
  void addCustomBook_afterAnAdd_theStagedPdfIsGone();

  void attachPdfToCurrentBook_storesIt();
  void attachPdfToCurrentBook_onACatalogBook_leavesItsMetadataAlone();
  void attachPdfToCurrentBook_onACustomBook_overridesTheMetadata();
  void attachPdfToCurrentBook_replacesAUserPdf();
  void attachPdfToCurrentBook_aServerPdf_isRefused();
  void attachPdfToCurrentBook_withoutASelection_isIgnored();
  void removePdfFromCurrentBook_dropsTheFileAndTheColumns();
  void removePdfFromCurrentBook_keepsTheCover();
  void removePdfFromCurrentBook_aServerPdf_isRefused();
  void removePdfFromCurrentBook_whenThereIsNone_isRefused();

  void setFilterCriteria_narrowsTheSearchModel();

  void errorMessage_startsEmpty();
  void setBookStatus_whenTheWriteFails_reportsIt();
  void toggleWishList_whenTheWriteFails_reportsIt();
  void updateReadingProgress_whenTheWriteFails_reportsIt();
  void restoreCachedProgress_whenTheWriteFails_reportsIt();
  void errorMessage_onlyChangesWhenTheTextDoes();

private:
  void dropTheBookBehindTheControllersBack();
  QString writePdf(const QString &name, int pageCount, const QString &title = {}, const QString &author = {},
                   const QString &subject = {}, const QString &printedIsbn = {});
  QString stagedPdfUrl(const QString &name, int pageCount, const QString &title = {}, const QString &author = {},
                       const QString &subject = {}, const QString &printedIsbn = {});
  qint64 addCustomWithPdf(const QString &pdfName, int pageCount, const QVariantMap &extraFields = {});

  QTemporaryDir _pdfDir;

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
  _controller = std::make_unique<BookController>(_library.books(), _library.files(), _listModel.get(), nullptr);

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

void BookControllerTest::readingJournalChanged_firesOnEverySessionWrite() {
  // Anything computed off the journal (the statistics page) has no other way
  // to learn a session appeared or went away.
  _controller->setCurrentBookIsbn(kIsbn);
  QSignalSpy journalSpy{_controller.get(), &BookController::readingJournalChanged};

  // The fixture book already stands at page 100.
  _controller->updateReadingProgress(200, 600);
  QCOMPARE(journalSpy.count(), 1);

  const auto sessions = _library.books()->getReadingSessions(kIsbn);
  QCOMPARE(sessions.size(), 1);
  _controller->deleteReadingSession(QString::number(sessions.first().id));
  QCOMPARE(journalSpy.count(), 2);
}

void BookControllerTest::readingJournalChanged_staysQuietWhenNothingWasWritten() {
  _controller->setCurrentBookIsbn(kIsbn);
  QSignalSpy journalSpy{_controller.get(), &BookController::readingJournalChanged};

  // A page behind the current position is refused, and a zero duration logs
  // no session at all.
  _controller->updateReadingProgress(0, 600);
  _controller->deleteReadingSession(u"not-a-number"_s);

  QCOMPARE(journalSpy.count(), 0);
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

void BookControllerTest::addCustomBook_storesTheFormAndOpensIt() {
  QSignalSpy savedSpy{_controller.get(), &BookController::bookSaved};
  QSignalSpy openSpy{_controller.get(), &BookController::bookOpenRequested};

  const QVariantMap form{
      {u"name"_s, u"My own notes"_s},
      {u"author"_s, u"Me"_s},
      {u"year"_s, 2024},
      {u"publisher"_s, u"Nobody"_s},
      {u"totalPages"_s, 120},
      {u"description"_s, u"Handwritten"_s},
      {u"coverUrl"_s, u"file:///c/cover.png"_s},
      {u"genres"_s, QStringList{u"Prose"_s}},
  };

  QVERIFY(_controller->addCustomBook(form));

  QCOMPARE(savedSpy.count(), 1);
  QCOMPARE(openSpy.count(), 1);
  QCOMPARE(_listModel->rowCount(), 3);

  const auto added = _controller->currentBookData();
  QVERIFY(added.isbn > 0);
  QCOMPARE(added.name, u"My own notes"_s);
  QCOMPARE(added.authorName, u"Me"_s);
  QCOMPARE(added.year, 2024);
  QCOMPARE(added.publisherName, u"Nobody"_s);
  QCOMPARE(added.totalPages, 120);
  QCOMPARE(added.description, u"Handwritten"_s);
  QCOMPARE(added.coverUrl, u"file:///c/cover.png"_s);
  QCOMPARE(added.genres, QStringList{u"Prose"_s});
  // Want-to-read, so the book is reachable from a category list right away.
  QCOMPARE(added.status, static_cast<int>(BookStatus::WantToRead));
  QVERIFY(added.isCustom);
}

void BookControllerTest::addCustomBook_withoutATitle_isRejected() {
  QVERIFY(!_controller->addCustomBook({{u"author"_s, u"Me"_s}}));

  QVERIFY(!_controller->errorMessage().isEmpty());
  QCOMPARE(_listModel->rowCount(), 2);
}

void BookControllerTest::addCustomBook_withoutAnAuthor_isRejected() {
  QVERIFY(!_controller->addCustomBook({{u"name"_s, u"My own notes"_s}}));

  QVERIFY(!_controller->errorMessage().isEmpty());
  QCOMPARE(_listModel->rowCount(), 2);
}

void BookControllerTest::addCustomBook_trimsTheText() {
  // No page count either: the DTO's 0 has to survive the schema's
  // `totalPages > 0` check as a NULL.
  QVERIFY(_controller->addCustomBook({{u"name"_s, u"  My own notes  "_s}, {u"author"_s, u"  Me  "_s}}));

  const auto added = _controller->currentBookData();
  QCOMPARE(added.name, u"My own notes"_s);
  QCOMPARE(added.authorName, u"Me"_s);
  QCOMPARE(added.totalPages, 0);
}

void BookControllerTest::addCustomBook_withoutAnIsbn_doesNotInventOne() {
  // Nothing 978-shaped is fabricated for a book that has no ISBN; SQLite keys
  // the row instead.
  QVERIFY(_controller->addCustomBook({{u"name"_s, u"My own notes"_s}, {u"author"_s, u"Me"_s}}));

  const qint64 key = _controller->currentBookIsbn();
  QVERIFY(key > 0);
  QVERIFY(key < 9'780'000'000'000LL);
}

void BookControllerTest::addCustomBook_keysEachBookSeparately() {
  QVERIFY(_controller->addCustomBook({{u"name"_s, u"First"_s}, {u"author"_s, u"Me"_s}}));
  const qint64 first = _controller->currentBookIsbn();

  QVERIFY(_controller->addCustomBook({{u"name"_s, u"Second"_s}, {u"author"_s, u"Me"_s}}));

  QVERIFY(_controller->currentBookIsbn() != first);
  QCOMPARE(_listModel->rowCount(), 4);
}

void BookControllerTest::addCustomBook_storesTheTypedIsbn() {
  QVERIFY(_controller->addCustomBook(
      {{u"name"_s, u"Refactoring"_s}, {u"author"_s, u"Me"_s}, {u"isbnText"_s, u"978-0-13-235088-4"_s}}));

  QCOMPARE(_controller->currentBookIsbn(), 9780132350884LL);
}

void BookControllerTest::addCustomBook_normalisesATypedIsbn10() {
  // The normalisation the catalog clients apply, so a hand-typed book keys the
  // same row an import of it would.
  QVERIFY(_controller->addCustomBook(
      {{u"name"_s, u"Refactoring"_s}, {u"author"_s, u"Me"_s}, {u"isbnText"_s, u"0-13-235088-2"_s}}));

  QCOMPARE(_controller->currentBookIsbn(), 9780132350884LL);
}

void BookControllerTest::addCustomBook_invalidTypedIsbn_isRefused() {
  // Refused rather than ignored: dropping it silently would file the book under
  // no ISBN while the user believes otherwise.
  QVERIFY(!_controller->addCustomBook(
      {{u"name"_s, u"Refactoring"_s}, {u"author"_s, u"Me"_s}, {u"isbnText"_s, u"978-0-13-235088-9"_s}}));

  QVERIFY(!_controller->errorMessage().isEmpty());
  QCOMPARE(_listModel->rowCount(), 2);
}

void BookControllerTest::addCustomBook_pdfIsbnOverridesTheTypedOne() {
  const QString url = stagedPdfUrl(u"isbn.pdf"_s, 3, {}, {}, {}, u"978-0-13-235088-4"_s);
  QVERIFY(!url.isEmpty());
  QVERIFY(_controller->stagePdf(url).value(u"ok"_s).toBool());

  QVERIFY(_controller->addCustomBook(
      {{u"name"_s, u"Refactoring"_s}, {u"author"_s, u"Me"_s}, {u"isbnText"_s, u"9780306406157"_s}}));

  QCOMPARE(_controller->currentBookIsbn(), 9780132350884LL);
}

void BookControllerTest::addCustomBook_anIsbnAlreadyInTheLibrary_isRefused() {
  QVERIFY(!_controller->addCustomBook(
      {{u"name"_s, u"Refactoring again"_s}, {u"author"_s, u"Me"_s}, {u"isbnText"_s, QString::number(kIsbn)}}));

  QVERIFY(!_controller->errorMessage().isEmpty());
  QCOMPARE(_listModel->rowCount(), 2);
}

void BookControllerTest::deleteCurrentBook_removesACustomBook() {
  QVERIFY(_controller->addCustomBook({{u"name"_s, u"My own notes"_s}, {u"author"_s, u"Me"_s}}));
  const qint64 isbn = _controller->currentBookIsbn();
  QSignalSpy savedSpy{_controller.get(), &BookController::bookSaved};

  QVERIFY(_controller->deleteCurrentBook());

  QCOMPARE(savedSpy.count(), 1);
  QCOMPARE(_controller->currentBookIsbn(), 0LL);
  QVERIFY(!_listModel->contains(isbn));
  QCOMPARE(_listModel->rowCount(), 2);
}

void BookControllerTest::deleteCurrentBook_aCatalogBook_isRefused() {
  // A catalog book only ever leaves a category — it can be found again online.
  _controller->setCurrentBookIsbn(kIsbn);

  QVERIFY(!_controller->deleteCurrentBook());

  QVERIFY(!_controller->errorMessage().isEmpty());
  QVERIFY(_listModel->contains(kIsbn));
  QCOMPARE(_controller->currentBookIsbn(), kIsbn);
}

void BookControllerTest::deleteCurrentBook_withoutASelection_isIgnored() {
  QVERIFY(!_controller->deleteCurrentBook());
  QCOMPARE(_listModel->rowCount(), 2);
}

void BookControllerTest::deleteCurrentBook_dropsTheParkedProgress() {
  QVERIFY(_controller->addCustomBook({{u"name"_s, u"My own notes"_s}, {u"author"_s, u"Me"_s}, {u"totalPages"_s, 100}}));
  const qint64 isbn = _controller->currentBookIsbn();
  _controller->updateReadingProgress(40, 60);
  _controller->moveInProgressToWantToRead();
  QVERIFY(_controller->hasCachedProgress());

  QVERIFY(_controller->deleteCurrentBook());

  _controller->setCurrentBookIsbn(isbn);
  QVERIFY(!_controller->hasCachedProgress());
}

QString BookControllerTest::writePdf(const QString &name, int pageCount, const QString &title, const QString &author,
                                     const QString &subject, const QString &printedIsbn) {
  if (!_pdfDir.isValid()) {
    return {};
  }
  QString path = QDir{_pdfDir.path()}.absoluteFilePath(name);
  if (!readary::tests::pdf::writeDocument(path, pageCount, title, author, subject, printedIsbn)) {
    return {};
  }
  return path;
}

QString BookControllerTest::stagedPdfUrl(const QString &name, int pageCount, const QString &title,
                                         const QString &author, const QString &subject, const QString &printedIsbn) {
  const QString path = writePdf(name, pageCount, title, author, subject, printedIsbn);
  if (path.isEmpty()) {
    return {};
  }
  // QML hands the controller a url, not a path.
  return QUrl::fromLocalFile(path).toString();
}

qint64 BookControllerTest::addCustomWithPdf(const QString &pdfName, int pageCount, const QVariantMap &extraFields) {
  const QString url = stagedPdfUrl(pdfName, pageCount);
  if (url.isEmpty()) {
    return 0;
  }
  if (!_controller->stagePdf(url).value(u"ok"_s).toBool()) {
    return 0;
  }

  QVariantMap fields{{u"name"_s, u"My own notes"_s}, {u"author"_s, u"Me"_s}};
  for (auto it = extraFields.constBegin(); it != extraFields.constEnd(); ++it) {
    fields.insert(it.key(), it.value());
  }
  if (!_controller->addCustomBook(fields)) {
    return 0;
  }
  return _controller->currentBookIsbn();
}

void BookControllerTest::deleteCurrentBook_dropsItsFiles() {
  const qint64 isbn = addCustomWithPdf(u"attached.pdf"_s, 12);
  QVERIFY(isbn > 0);
  QVERIFY(QFile::exists(_library.files()->pdfPath(isbn)));
  QVERIFY(QFile::exists(_library.files()->coverPath(isbn)));

  QVERIFY(_controller->deleteCurrentBook());

  QVERIFY(!QFile::exists(_library.files()->pdfPath(isbn)));
  QVERIFY(!QFile::exists(_library.files()->coverPath(isbn)));
}

void BookControllerTest::stagePdf_reportsWhatThePdfSays() {
  const QString url = stagedPdfUrl(u"staged.pdf"_s, 7, u"Refactoring"_s, u"Martin Fowler"_s);
  QVERIFY(!url.isEmpty());

  const QVariantMap info = _controller->stagePdf(url);

  QVERIFY(info.value(u"ok"_s).toBool());
  QCOMPARE(info.value(u"pageCount"_s).toInt(), 7);
  QCOMPARE(info.value(u"title"_s).toString(), u"Refactoring"_s);
  QCOMPARE(info.value(u"author"_s).toString(), u"Martin Fowler"_s);
}

void BookControllerTest::stagePdf_handsBackTheCoverInline() {
  // The form has to show the rendered first page before the book exists, and the
  // stored cover is named after an isbn that is only assigned on commit — so the
  // render travels inline rather than as a path.
  const QString url = stagedPdfUrl(u"cover.pdf"_s, 2);
  QVERIFY(!url.isEmpty());

  const QString preview = _controller->stagePdf(url).value(u"coverPreview"_s).toString();

  QVERIFY(preview.startsWith(u"data:image/png;base64,"_s));
  const QByteArray png = QByteArray::fromBase64(preview.mid(QStringView{u"data:image/png;base64,"}.size()).toLatin1());
  QVERIFY(png.startsWith(QByteArray::fromHex("89504E47")));
  QImage decoded;
  QVERIFY(decoded.loadFromData(png, "PNG"));
  QVERIFY(!decoded.isNull());
}

void BookControllerTest::stagePdf_aFileThatIsNotAPdf_isRejected() {
  const QString path = QDir{_pdfDir.path()}.absoluteFilePath(u"notes.txt"_s);
  QFile file(path);
  QVERIFY(file.open(QIODevice::WriteOnly));
  QVERIFY(file.write("not a pdf") > 0);
  file.close();

  const QVariantMap info = _controller->stagePdf(QUrl::fromLocalFile(path).toString());

  QVERIFY(!info.value(u"ok"_s).toBool());
  QVERIFY(!_controller->errorMessage().isEmpty());
}

void BookControllerTest::clearStagedPdf_dropsIt() {
  const QString url = stagedPdfUrl(u"staged.pdf"_s, 7);
  QVERIFY(!url.isEmpty());
  QVERIFY(_controller->stagePdf(url).value(u"ok"_s).toBool());

  _controller->clearStagedPdf();

  QVERIFY(_controller->addCustomBook({{u"name"_s, u"Typed"_s}, {u"author"_s, u"Me"_s}}));
  const auto added = _controller->currentBookData();
  QCOMPARE(added.pdfSource, static_cast<int>(PdfSource::None));
  QVERIFY(added.pdfPath.isEmpty());
  QCOMPARE(added.totalPages, 0);
}

void BookControllerTest::addCustomBook_withAStagedPdf_storesItAndTheCover() {
  const qint64 isbn = addCustomWithPdf(u"attached.pdf"_s, 12);
  QVERIFY(isbn > 0);

  const auto added = _controller->currentBookData();
  QCOMPARE(added.pdfSource, static_cast<int>(PdfSource::User));
  QCOMPARE(added.pdfPath, _library.files()->pdfPath(isbn));
  QVERIFY(QFile::exists(added.pdfPath));
  // The rendered first page becomes the cover, as a local file url.
  QVERIFY(added.coverUrl.startsWith(u"file://"_s));
  QVERIFY(QFile::exists(_library.files()->coverPath(isbn)));
}

void BookControllerTest::addCustomBook_copiesAPickedCoverIntoTheStore() {
  const QString source = QDir{_pdfDir.path()}.absoluteFilePath(u"cover.png"_s);
  QImage picked(20, 30, QImage::Format_RGB32);
  picked.fill(Qt::blue);
  QVERIFY(picked.save(source, "PNG"));

  QVERIFY(_controller->addCustomBook({{u"name"_s, u"My own notes"_s},
                                      {u"author"_s, u"Me"_s},
                                      {u"coverUrl"_s, QUrl::fromLocalFile(source).toString()}}));

  const qint64 key = _controller->currentBookIsbn();
  QVERIFY(QFile::exists(_library.files()->coverPath(key)));
  QCOMPARE(_controller->currentBookData().coverUrl, _library.files()->coverUrl(key));
}

void BookControllerTest::addCustomBook_pickedCoverSurvivesTheOriginalBeingDeleted() {
  // The point of copying rather than referencing — and on Android the picked
  // uri's read grant does not outlive the app anyway.
  const QString source = QDir{_pdfDir.path()}.absoluteFilePath(u"cover.png"_s);
  QImage picked(20, 30, QImage::Format_RGB32);
  picked.fill(Qt::green);
  QVERIFY(picked.save(source, "PNG"));
  QVERIFY(_controller->addCustomBook({{u"name"_s, u"My own notes"_s},
                                      {u"author"_s, u"Me"_s},
                                      {u"coverUrl"_s, QUrl::fromLocalFile(source).toString()}}));
  const qint64 key = _controller->currentBookIsbn();

  QVERIFY(QFile::remove(source));

  QVERIFY(QFile::exists(_library.files()->coverPath(key)));
  QVERIFY(!QImage{_library.files()->coverPath(key)}.isNull());
}

void BookControllerTest::addCustomBook_pdfPageCountOverridesTheForm() {
  const QString url = stagedPdfUrl(u"pages.pdf"_s, 9);
  QVERIFY(!url.isEmpty());
  QVERIFY(_controller->stagePdf(url).value(u"ok"_s).toBool());

  QVERIFY(_controller->addCustomBook({{u"name"_s, u"My own notes"_s}, {u"author"_s, u"Me"_s}, {u"totalPages"_s, 999}}));

  QCOMPARE(_controller->currentBookData().totalPages, 9);
}

void BookControllerTest::addCustomBook_pdfMetadataOverridesTheForm() {
  const QString url = stagedPdfUrl(u"meta.pdf"_s, 4, u"Refactoring"_s, u"Martin Fowler"_s, u"From the pdf"_s);
  QVERIFY(!url.isEmpty());
  QVERIFY(_controller->stagePdf(url).value(u"ok"_s).toBool());

  QVERIFY(_controller->addCustomBook(
      {{u"name"_s, u"Typed title"_s}, {u"author"_s, u"Typed author"_s}, {u"description"_s, u"Typed description"_s}}));

  const auto added = _controller->currentBookData();
  QCOMPARE(added.name, u"Refactoring"_s);
  QCOMPARE(added.authorName, u"Martin Fowler"_s);
  QCOMPARE(added.description, u"From the pdf"_s);
}

void BookControllerTest::addCustomBook_pdfWithoutMetadata_keepsTheTypedText() {
  // A blank /Info dictionary must not wipe what the user typed — an empty
  // override would be a loss, not a correction.
  const QString url = stagedPdfUrl(u"bare.pdf"_s, 5);
  QVERIFY(!url.isEmpty());
  QVERIFY(_controller->stagePdf(url).value(u"ok"_s).toBool());

  QVERIFY(_controller->addCustomBook({{u"name"_s, u"Typed title"_s}, {u"author"_s, u"Typed author"_s}}));

  const auto added = _controller->currentBookData();
  QCOMPARE(added.name, u"Typed title"_s);
  QCOMPARE(added.authorName, u"Typed author"_s);
  QCOMPARE(added.totalPages, 5);
}

void BookControllerTest::addCustomBook_pdfSuppliesTheTitleTheFormLeftBlank() {
  const QString url = stagedPdfUrl(u"titled.pdf"_s, 3, u"Refactoring"_s, u"Martin Fowler"_s);
  QVERIFY(!url.isEmpty());
  QVERIFY(_controller->stagePdf(url).value(u"ok"_s).toBool());

  // Nothing typed at all: the pdf alone satisfies the title/author requirement.
  QVERIFY(_controller->addCustomBook({}));

  const auto added = _controller->currentBookData();
  QCOMPARE(added.name, u"Refactoring"_s);
  QCOMPARE(added.authorName, u"Martin Fowler"_s);
}

void BookControllerTest::addCustomBook_afterAnAdd_theStagedPdfIsGone() {
  QVERIFY(addCustomWithPdf(u"first.pdf"_s, 6) > 0);

  QVERIFY(_controller->addCustomBook({{u"name"_s, u"Second"_s}, {u"author"_s, u"Me"_s}}));

  const auto second = _controller->currentBookData();
  QCOMPARE(second.pdfSource, static_cast<int>(PdfSource::None));
  QVERIFY(second.pdfPath.isEmpty());
}

void BookControllerTest::attachPdfToCurrentBook_storesIt() {
  _controller->setCurrentBookIsbn(kIsbn);
  const QString url = stagedPdfUrl(u"attach.pdf"_s, 11);
  QVERIFY(!url.isEmpty());

  QVERIFY(_controller->attachPdfToCurrentBook(url));

  const auto book = _controller->currentBookData();
  QCOMPARE(book.pdfSource, static_cast<int>(PdfSource::User));
  QCOMPARE(book.pdfPath, _library.files()->pdfPath(kIsbn));
  QVERIFY(QFile::exists(book.pdfPath));
}

void BookControllerTest::attachPdfToCurrentBook_onACatalogBook_leavesItsMetadataAlone() {
  // A catalog book took its fields from the catalog, not from a form — the pdf
  // has no better claim on them.
  _controller->setCurrentBookIsbn(kIsbn);
  const auto before = _controller->currentBookData();
  const QString url = stagedPdfUrl(u"attach.pdf"_s, 11, u"Pdf Title"_s, u"Pdf Author"_s, u"Pdf subject"_s);
  QVERIFY(!url.isEmpty());

  QVERIFY(_controller->attachPdfToCurrentBook(url));

  const auto after = _controller->currentBookData();
  QCOMPARE(after.name, before.name);
  QCOMPARE(after.authorName, before.authorName);
  QCOMPARE(after.totalPages, before.totalPages);
  QCOMPARE(after.description, before.description);
}

void BookControllerTest::attachPdfToCurrentBook_onACustomBook_overridesTheMetadata() {
  QVERIFY(_controller->addCustomBook({{u"name"_s, u"Typed"_s}, {u"author"_s, u"Me"_s}, {u"totalPages"_s, 100}}));
  const QString url = stagedPdfUrl(u"later.pdf"_s, 42, u"Pdf Title"_s, u"Pdf Author"_s);
  QVERIFY(!url.isEmpty());

  QVERIFY(_controller->attachPdfToCurrentBook(url));

  const auto book = _controller->currentBookData();
  QCOMPARE(book.totalPages, 42);
  QCOMPARE(book.name, u"Pdf Title"_s);
  QCOMPARE(book.authorName, u"Pdf Author"_s);
}

void BookControllerTest::attachPdfToCurrentBook_replacesAUserPdf() {
  _controller->setCurrentBookIsbn(kIsbn);
  QVERIFY(_controller->attachPdfToCurrentBook(stagedPdfUrl(u"first.pdf"_s, 3)));

  QVERIFY(_controller->attachPdfToCurrentBook(stagedPdfUrl(u"second.pdf"_s, 8)));

  const auto book = _controller->currentBookData();
  QCOMPARE(book.pdfSource, static_cast<int>(PdfSource::User));
  QCOMPARE(book.pdfPath, _library.files()->pdfPath(kIsbn));
  // Still the catalog's page count: kIsbn is not a custom book, so neither pdf
  // got to override it.
  QCOMPARE(book.totalPages, 448);
}

void BookControllerTest::attachPdfToCurrentBook_aServerPdf_isRefused() {
  BookDTO locked = makeBook(kUnknownIsbn, u"Locked"_s, BookStatus::WantToRead);
  locked.pdfPath = u"C:/from/the/catalog.pdf"_s;
  locked.pdfSource = PdfSource::Server;
  QCOMPARE(_library.books()->addBook(locked), kUnknownIsbn);
  _listModel->refresh();
  _controller->setCurrentBookIsbn(kUnknownIsbn);

  QVERIFY(!_controller->attachPdfToCurrentBook(stagedPdfUrl(u"mine.pdf"_s, 3)));

  QVERIFY(!_controller->errorMessage().isEmpty());
  QCOMPARE(_controller->currentBookData().pdfPath, u"C:/from/the/catalog.pdf"_s);
}

void BookControllerTest::attachPdfToCurrentBook_withoutASelection_isIgnored() {
  QVERIFY(!_controller->attachPdfToCurrentBook(stagedPdfUrl(u"mine.pdf"_s, 3)));
}

void BookControllerTest::removePdfFromCurrentBook_dropsTheFileAndTheColumns() {
  const qint64 isbn = addCustomWithPdf(u"attached.pdf"_s, 12);
  QVERIFY(isbn > 0);

  QVERIFY(_controller->removePdfFromCurrentBook());

  const auto book = _controller->currentBookData();
  QVERIFY(book.pdfPath.isEmpty());
  QCOMPARE(book.pdfSource, static_cast<int>(PdfSource::None));
  QVERIFY(!QFile::exists(_library.files()->pdfPath(isbn)));
}

void BookControllerTest::removePdfFromCurrentBook_keepsTheCover() {
  // Once rendered, the cover is the book's own picture.
  const qint64 isbn = addCustomWithPdf(u"attached.pdf"_s, 12);
  QVERIFY(isbn > 0);
  const QString coverUrl = _controller->currentBookData().coverUrl;
  QVERIFY(!coverUrl.isEmpty());

  QVERIFY(_controller->removePdfFromCurrentBook());

  QCOMPARE(_controller->currentBookData().coverUrl, coverUrl);
  QVERIFY(QFile::exists(_library.files()->coverPath(isbn)));
}

void BookControllerTest::removePdfFromCurrentBook_aServerPdf_isRefused() {
  BookDTO locked = makeBook(kUnknownIsbn, u"Locked"_s, BookStatus::WantToRead);
  locked.pdfPath = u"C:/from/the/catalog.pdf"_s;
  locked.pdfSource = PdfSource::Server;
  QCOMPARE(_library.books()->addBook(locked), kUnknownIsbn);
  _listModel->refresh();
  _controller->setCurrentBookIsbn(kUnknownIsbn);

  QVERIFY(!_controller->removePdfFromCurrentBook());

  QVERIFY(!_controller->errorMessage().isEmpty());
  QCOMPARE(_controller->currentBookData().pdfSource, static_cast<int>(PdfSource::Server));
}

void BookControllerTest::removePdfFromCurrentBook_whenThereIsNone_isRefused() {
  _controller->setCurrentBookIsbn(kIsbn);

  QVERIFY(!_controller->removePdfFromCurrentBook());
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
