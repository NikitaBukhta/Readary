#include "controllers/GlobalBookSearchController.hpp"
#include "api/bookSearch/IBookSearchAPI.hpp"
#include "models/books/BookListModelBase.hpp"
#include "models/books/GlobalBookSearchListModel.hpp"
#include "services/BookDTO.hpp"

#include <QAbstractItemModel>
#include <QList>
#include <QSettings>
#include <QStandardPaths>
#include <QString>
#include <QTest>

using readary::controllers::GlobalBookSearchController;
using readary::services::BookDTO;

namespace {

BookDTO makeBook(qint64 isbn, const QString &name) {
  BookDTO book;
  book.isbn = isbn;
  book.name = name;
  book.totalPages = 200;
  return book;
}

// Minimal IBookSearchAPI stand-in: records how it was called and lets the test
// deliver results synchronously via the inherited searchListUpdated signal.
class FakeSearchApi : public readary::api::IBookSearchAPI {
public:
  explicit FakeSearchApi(QObject *parent = nullptr) : IBookSearchAPI(parent) {}

  void search(const readary::api::BookSearchFields &params) override {
    ++searchCalls;
    lastPage = params.page;
    lastName = params.name;
  }
  void searchByISBN(qint64 isbn) override {
    ++isbnCalls;
    lastIsbn = isbn;
  }
  void fetchDescription(const QString &workKey) override {
    ++descriptionCalls;
    lastWorkKey = workKey;
  }

  void deliver(const QList<BookDTO> &books, bool hasMore) { emit searchListUpdated(books, hasMore); }
  void deliverDescription(const QString &workKey, const QString &description) {
    emit descriptionReady(workKey, description);
  }

  int searchCalls = 0;
  int isbnCalls = 0;
  int descriptionCalls = 0;
  int lastPage = 0;
  qint64 lastIsbn = 0;
  QString lastName;
  QString lastWorkKey;
};

qint64 isbnAt(const QAbstractItemModel *model, int row) {
  return model->data(model->index(row, 0), readary::models::BookListModelBase::IsbnRole).toLongLong();
}

} // namespace

class GlobalBookSearchControllerTest : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void init();

  void freshSearch_requestsPageOne_thenPopulatesModelOnResults();
  void repeatSearch_servedFromMemoryCache_noNetwork();
  void repeatSearch_isCaseInsensitive();
  void loadMore_fetchesNextPage_andAppends();
  void loadMore_whenNoMorePages_doesNothing();
  void loadMore_withoutActiveQuery_doesNothing();
  void isbnQuery_routesToSearchByIsbn();
  void emptyQuery_isIgnored();
  void openBook_knownIsbn_emitsImportRequest();
  void openBook_unknownIsbn_doesNotEmit();
  void persistedSearch_reusedByNewInstance_noNetwork();
  void switchingQuery_replacesModelContents();
  void loadMore_acrossPages_accumulatesThenStopsWhenNoMore();
  void loadMore_whileRequestInFlight_isIgnored();
  void cachedQuery_resumesPaginationFromNextPage();
  void search_withoutApi_doesNotCrash();
  void openBook_withWorkKey_fetchesDescriptionThenImportsWithIt();
  void openBook_withoutWorkKey_importsWithoutFetch();
  void onDescriptionReady_forStaleWorkKey_isIgnored();
};

void GlobalBookSearchControllerTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  QCoreApplication::setOrganizationName("DarieszzBooksTests");
  QCoreApplication::setOrganizationDomain("tests.darieszzbooks.local");
  QCoreApplication::setApplicationName("GlobalBookSearchControllerTest");
}

void GlobalBookSearchControllerTest::init() {
  QSettings settings; // start every test with an empty persistent cache
  settings.clear();
  settings.sync();
}

void GlobalBookSearchControllerTest::freshSearch_requestsPageOne_thenPopulatesModelOnResults() {
  GlobalBookSearchController ctrl(nullptr);
  FakeSearchApi api;
  ctrl.setBookSearchAPI(&api);

  ctrl.search(QStringLiteral("tolkien"));
  QCOMPARE(api.searchCalls, 1);
  QCOMPARE(api.lastPage, 1);
  QCOMPARE(ctrl.resultsModel()->rowCount(), 0); // nothing shown until results arrive

  api.deliver({makeBook(1, QStringLiteral("a")), makeBook(2, QStringLiteral("b"))}, true);
  QCOMPARE(ctrl.resultsModel()->rowCount(), 2);
}

void GlobalBookSearchControllerTest::repeatSearch_servedFromMemoryCache_noNetwork() {
  GlobalBookSearchController ctrl(nullptr);
  FakeSearchApi api;
  ctrl.setBookSearchAPI(&api);

  ctrl.search(QStringLiteral("tolkien"));
  api.deliver({makeBook(1, QStringLiteral("a"))}, true);
  QCOMPARE(api.searchCalls, 1);

  ctrl.search(QStringLiteral("tolkien"));
  QCOMPARE(api.searchCalls, 1); // reused from memory, no new request
  QCOMPARE(ctrl.resultsModel()->rowCount(), 1);
}

void GlobalBookSearchControllerTest::repeatSearch_isCaseInsensitive() {
  GlobalBookSearchController ctrl(nullptr);
  FakeSearchApi api;
  ctrl.setBookSearchAPI(&api);

  ctrl.search(QStringLiteral("Tolkien"));
  api.deliver({makeBook(1, QStringLiteral("a"))}, true);
  QCOMPARE(api.searchCalls, 1);

  ctrl.search(QStringLiteral("  tolkien ")); // different case + whitespace
  QCOMPARE(api.searchCalls, 1);              // same normalized key → cache hit
}

void GlobalBookSearchControllerTest::loadMore_fetchesNextPage_andAppends() {
  GlobalBookSearchController ctrl(nullptr);
  FakeSearchApi api;
  ctrl.setBookSearchAPI(&api);

  ctrl.search(QStringLiteral("tolkien"));
  api.deliver({makeBook(1, QStringLiteral("a")), makeBook(2, QStringLiteral("b"))}, true);
  QCOMPARE(ctrl.resultsModel()->rowCount(), 2);

  ctrl.loadMore();
  QCOMPARE(api.searchCalls, 2);
  QCOMPARE(api.lastPage, 2);

  api.deliver({makeBook(3, QStringLiteral("c"))}, true);
  QCOMPARE(ctrl.resultsModel()->rowCount(), 3); // appended, not replaced
}

void GlobalBookSearchControllerTest::loadMore_whenNoMorePages_doesNothing() {
  GlobalBookSearchController ctrl(nullptr);
  FakeSearchApi api;
  ctrl.setBookSearchAPI(&api);

  ctrl.search(QStringLiteral("tolkien"));
  api.deliver({makeBook(1, QStringLiteral("a"))}, false); // hasMore = false

  ctrl.loadMore();
  QCOMPARE(api.searchCalls, 1); // no further page requested
}

void GlobalBookSearchControllerTest::loadMore_withoutActiveQuery_doesNothing() {
  GlobalBookSearchController ctrl(nullptr);
  FakeSearchApi api;
  ctrl.setBookSearchAPI(&api);

  ctrl.loadMore();
  QCOMPARE(api.searchCalls, 0);
  QCOMPARE(api.isbnCalls, 0);
}

void GlobalBookSearchControllerTest::isbnQuery_routesToSearchByIsbn() {
  GlobalBookSearchController ctrl(nullptr);
  FakeSearchApi api;
  ctrl.setBookSearchAPI(&api);

  ctrl.search(QStringLiteral("9780007487301")); // valid ISBN-13
  QCOMPARE(api.isbnCalls, 1);
  QCOMPARE(api.searchCalls, 0);
  QCOMPARE(api.lastIsbn, static_cast<qint64>(9780007487301));
}

void GlobalBookSearchControllerTest::emptyQuery_isIgnored() {
  GlobalBookSearchController ctrl(nullptr);
  FakeSearchApi api;
  ctrl.setBookSearchAPI(&api);

  ctrl.search(QStringLiteral("   "));
  QCOMPARE(api.searchCalls, 0);
  QCOMPARE(api.isbnCalls, 0);
}

void GlobalBookSearchControllerTest::openBook_knownIsbn_emitsImportRequest() {
  GlobalBookSearchController ctrl(nullptr);
  FakeSearchApi api;
  ctrl.setBookSearchAPI(&api);

  ctrl.search(QStringLiteral("tolkien"));
  api.deliver({makeBook(111, QStringLiteral("a"))}, false);

  int importCount = 0;
  qint64 importedIsbn = 0;
  QObject::connect(&ctrl, &GlobalBookSearchController::bookImportRequested, &ctrl, [&](const BookDTO &book) {
    ++importCount;
    importedIsbn = book.isbn;
  });

  ctrl.openBook(111);
  QCOMPARE(importCount, 1);
  QCOMPARE(importedIsbn, static_cast<qint64>(111));
}

void GlobalBookSearchControllerTest::openBook_unknownIsbn_doesNotEmit() {
  GlobalBookSearchController ctrl(nullptr);
  FakeSearchApi api;
  ctrl.setBookSearchAPI(&api);

  ctrl.search(QStringLiteral("tolkien"));
  api.deliver({makeBook(111, QStringLiteral("a"))}, false);

  int importCount = 0;
  QObject::connect(&ctrl, &GlobalBookSearchController::bookImportRequested, &ctrl,
                   [&](const BookDTO &) { ++importCount; });

  ctrl.openBook(999); // not in results
  QCOMPARE(importCount, 0);
}

void GlobalBookSearchControllerTest::persistedSearch_reusedByNewInstance_noNetwork() {
  {
    GlobalBookSearchController ctrl(nullptr);
    FakeSearchApi api;
    ctrl.setBookSearchAPI(&api);
    ctrl.search(QStringLiteral("persisted"));
    api.deliver({makeBook(1, QStringLiteral("a")), makeBook(2, QStringLiteral("b"))}, true);
    QCOMPARE(api.searchCalls, 1);
  }

  // A brand-new controller has an empty in-memory cache (simulates an app restart),
  // but the persisted entry should serve the same query without any network call.
  GlobalBookSearchController ctrl2(nullptr);
  FakeSearchApi api2;
  ctrl2.setBookSearchAPI(&api2);
  ctrl2.search(QStringLiteral("persisted"));
  QCOMPARE(api2.searchCalls, 0);
  QCOMPARE(ctrl2.resultsModel()->rowCount(), 2);
}

void GlobalBookSearchControllerTest::switchingQuery_replacesModelContents() {
  GlobalBookSearchController ctrl(nullptr);
  FakeSearchApi api;
  ctrl.setBookSearchAPI(&api);

  ctrl.search(QStringLiteral("a"));
  api.deliver({makeBook(1, QStringLiteral("a1")), makeBook(2, QStringLiteral("a2"))}, false);
  QCOMPARE(ctrl.resultsModel()->rowCount(), 2);

  ctrl.search(QStringLiteral("b"));
  api.deliver({makeBook(3, QStringLiteral("b1"))}, false);
  QCOMPARE(ctrl.resultsModel()->rowCount(), 1);
  QCOMPARE(isbnAt(ctrl.resultsModel(), 0), static_cast<qint64>(3)); // b's book, not a's
}

void GlobalBookSearchControllerTest::loadMore_acrossPages_accumulatesThenStopsWhenNoMore() {
  GlobalBookSearchController ctrl(nullptr);
  FakeSearchApi api;
  ctrl.setBookSearchAPI(&api);

  ctrl.search(QStringLiteral("q"));
  api.deliver({makeBook(1, QStringLiteral("a")), makeBook(2, QStringLiteral("b"))}, true);

  ctrl.loadMore();
  api.deliver({makeBook(3, QStringLiteral("c")), makeBook(4, QStringLiteral("d"))}, true);

  ctrl.loadMore();
  api.deliver({makeBook(5, QStringLiteral("e"))}, false); // last page
  QCOMPARE(ctrl.resultsModel()->rowCount(), 5);
  QCOMPARE(api.searchCalls, 3);

  ctrl.loadMore(); // hasMore is false now → no further request
  QCOMPARE(api.searchCalls, 3);
}

void GlobalBookSearchControllerTest::loadMore_whileRequestInFlight_isIgnored() {
  GlobalBookSearchController ctrl(nullptr);
  FakeSearchApi api;
  ctrl.setBookSearchAPI(&api);

  ctrl.search(QStringLiteral("q")); // page 1 in flight (results not delivered yet)
  QCOMPARE(api.searchCalls, 1);

  ctrl.loadMore(); // guarded while loading
  QCOMPARE(api.searchCalls, 1);
}

void GlobalBookSearchControllerTest::cachedQuery_resumesPaginationFromNextPage() {
  GlobalBookSearchController ctrl(nullptr);
  FakeSearchApi api;
  ctrl.setBookSearchAPI(&api);

  ctrl.search(QStringLiteral("a"));
  api.deliver({makeBook(1, QStringLiteral("a1")), makeBook(2, QStringLiteral("a2"))}, true);
  ctrl.loadMore();
  api.deliver({makeBook(3, QStringLiteral("a3"))}, true); // a now has 2 pages loaded

  ctrl.search(QStringLiteral("b")); // switch away
  api.deliver({makeBook(9, QStringLiteral("b1"))}, true);

  ctrl.search(QStringLiteral("a")); // back to a → served from cache (3 books)
  QCOMPARE(ctrl.resultsModel()->rowCount(), 3);

  ctrl.loadMore(); // must resume at page 3, not restart at page 1
  QCOMPARE(api.lastPage, 3);
}

void GlobalBookSearchControllerTest::search_withoutApi_doesNotCrash() {
  GlobalBookSearchController ctrl(nullptr); // no setBookSearchAPI()
  ctrl.search(QStringLiteral("orphan"));
  QCOMPARE(ctrl.resultsModel()->rowCount(), 0);
}

void GlobalBookSearchControllerTest::openBook_withWorkKey_fetchesDescriptionThenImportsWithIt() {
  GlobalBookSearchController ctrl(nullptr);
  FakeSearchApi api;
  ctrl.setBookSearchAPI(&api);

  BookDTO result = makeBook(111, QStringLiteral("a"));
  result.workKey = QStringLiteral("/works/OL1W");
  ctrl.search(QStringLiteral("tolkien"));
  api.deliver({result}, false);

  int importCount = 0;
  BookDTO imported;
  QObject::connect(&ctrl, &GlobalBookSearchController::bookImportRequested, &ctrl, [&](const BookDTO &book) {
    ++importCount;
    imported = book;
  });

  ctrl.openBook(111);
  QCOMPARE(api.descriptionCalls, 1);
  QCOMPARE(api.lastWorkKey, QStringLiteral("/works/OL1W"));
  QCOMPARE(importCount, 0); // deferred until the description arrives

  api.deliverDescription(QStringLiteral("/works/OL1W"), QStringLiteral("What the book is about."));
  QCOMPARE(importCount, 1);
  QCOMPARE(imported.description, QStringLiteral("What the book is about."));
}

void GlobalBookSearchControllerTest::openBook_withoutWorkKey_importsWithoutFetch() {
  GlobalBookSearchController ctrl(nullptr);
  FakeSearchApi api;
  ctrl.setBookSearchAPI(&api);

  ctrl.search(QStringLiteral("tolkien"));
  api.deliver({makeBook(111, QStringLiteral("a"))}, false); // no workKey

  int importCount = 0;
  QObject::connect(&ctrl, &GlobalBookSearchController::bookImportRequested, &ctrl,
                   [&](const BookDTO &) { ++importCount; });

  ctrl.openBook(111);
  QCOMPARE(api.descriptionCalls, 0);
  QCOMPARE(importCount, 1);
}

void GlobalBookSearchControllerTest::onDescriptionReady_forStaleWorkKey_isIgnored() {
  GlobalBookSearchController ctrl(nullptr);
  FakeSearchApi api;
  ctrl.setBookSearchAPI(&api);

  BookDTO result = makeBook(111, QStringLiteral("a"));
  result.workKey = QStringLiteral("/works/OL1W");
  ctrl.search(QStringLiteral("tolkien"));
  api.deliver({result}, false);

  int importCount = 0;
  QObject::connect(&ctrl, &GlobalBookSearchController::bookImportRequested, &ctrl,
                   [&](const BookDTO &) { ++importCount; });

  ctrl.openBook(111);
  api.deliverDescription(QStringLiteral("/works/OTHER"), QStringLiteral("x")); // mismatched key
  QCOMPARE(importCount, 0);

  api.deliverDescription(QStringLiteral("/works/OL1W"), QStringLiteral("real"));
  QCOMPARE(importCount, 1);
}

QTEST_GUILESS_MAIN(GlobalBookSearchControllerTest)
#include "GlobalBookSearchControllerTest.moc"
