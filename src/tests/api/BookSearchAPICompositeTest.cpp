#include "api/bookSearch/BookSearchAPIComposite.hpp"
#include "api/bookSearch/IBookSearchAPI.hpp"
#include "services/BookDTO.hpp"
#include "support/SearchCapture.hpp"

#include <QList>
#include <QString>
#include <QTest>

using Qt::StringLiterals::operator""_s;

using readary::api::BookSearchAPIComposite;
using readary::api::BookSearchFields;
using readary::api::IBookSearchAPI;
using readary::services::BookDTO;
using readary::tests::observeSearch;
using readary::tests::SearchResult;

namespace {

// Synchronous stand-in: echoes a preconfigured result list the moment it is
// queried, and counts how often it was asked, so the composite's fallback
// routing can be asserted deterministically without any network.
class FakeSearchAPI : public IBookSearchAPI {
public:
  explicit FakeSearchAPI(QList<BookDTO> results, QObject *parent = nullptr)
      : IBookSearchAPI{parent}, _results{std::move(results)} {}

  void search(const BookSearchFields &params) override {
    Q_UNUSED(params)
    ++searchCount;
    emit searchListUpdated(_results, false);
  }
  void searchByISBN(qint64 isbn) override {
    Q_UNUSED(isbn)
    ++searchCount;
    emit searchListUpdated(_results, false);
  }
  void fetchDescription(const QString &workKey) override {
    ++fetchCount;
    emit descriptionReady(workKey, u"desc"_s);
  }

  int searchCount{0};
  int fetchCount{0};

private:
  QList<BookDTO> _results;
};

// Deferred stand-in: a search() call is queued rather than answered, and each
// flushReply() releases exactly one queued reply. Lets a test interleave two
// overlapping searches to exercise the composite's stale-reply handling.
class DeferredSearchAPI : public IBookSearchAPI {
public:
  explicit DeferredSearchAPI(QList<BookDTO> results, QObject *parent = nullptr)
      : IBookSearchAPI{parent}, _results{std::move(results)} {}

  void search(const BookSearchFields &params) override {
    Q_UNUSED(params)
    ++searchCount;
    ++_queued;
  }
  void searchByISBN(qint64 isbn) override {
    Q_UNUSED(isbn)
    ++searchCount;
    ++_queued;
  }
  void fetchDescription(const QString &workKey) override { Q_UNUSED(workKey) }

  void flushReply() {
    if (_queued > 0) {
      --_queued;
      emit searchListUpdated(_results, false);
    }
  }

  int searchCount{0};

private:
  QList<BookDTO> _results;
  int _queued{0};
};

BookDTO makeBook(qint64 isbn) {
  BookDTO book;
  book.isbn = isbn;
  return book;
}

} // namespace

class BookSearchAPICompositeTest : public QObject {
  Q_OBJECT

private slots:
  void primaryWithResults_doesNotQueryFallback();
  void primaryEmpty_queriesFallback();
  void bothEmpty_emitsEmptyResult();
  void overlappingSearch_ignoresStalePrimaryReply_queriesFallbackOnce();
  void fetchDescription_reachesFallback();
  void criteria_dropResultsTheSourceDidNotHonour();
  void criteria_rejectingEveryPrimaryResult_queriesFallback();
};

void BookSearchAPICompositeTest::primaryWithResults_doesNotQueryFallback() {
  FakeSearchAPI primary{{makeBook(111)}};
  FakeSearchAPI fallback{{makeBook(999)}};

  BookSearchAPIComposite composite{{&primary}};
  composite.setFallbackAPI(&fallback);

  SearchResult cap;
  observeSearch(composite, cap);

  composite.search(BookSearchFields{.isbn = 0, .name = u"anything"_s, .author = u"anything"_s});

  QVERIFY(cap.received);
  QCOMPARE(cap.books.size(), 1);
  QCOMPARE(cap.books.first().isbn, 111LL);
  QCOMPARE(primary.searchCount, 1);
  QCOMPARE(fallback.searchCount, 0);
}

void BookSearchAPICompositeTest::primaryEmpty_queriesFallback() {
  FakeSearchAPI primary{{}};
  FakeSearchAPI fallback{{makeBook(222), makeBook(333)}};

  BookSearchAPIComposite composite{{&primary}};
  composite.setFallbackAPI(&fallback);

  SearchResult cap;
  observeSearch(composite, cap);

  composite.search(BookSearchFields{.isbn = 0, .name = u"missing"_s, .author = u"missing"_s});

  QVERIFY(cap.received);
  QCOMPARE(cap.books.size(), 2);
  QCOMPARE(primary.searchCount, 1);
  QCOMPARE(fallback.searchCount, 1);
}

void BookSearchAPICompositeTest::bothEmpty_emitsEmptyResult() {
  FakeSearchAPI primary{{}};
  FakeSearchAPI fallback{{}};

  BookSearchAPIComposite composite{{&primary}};
  composite.setFallbackAPI(&fallback);

  SearchResult cap;
  observeSearch(composite, cap);

  composite.searchByISBN(9780306406157LL);

  QVERIFY(cap.received);
  QCOMPARE(cap.books.size(), 0);
  QCOMPARE(primary.searchCount, 1);
  QCOMPARE(fallback.searchCount, 1);
}

void BookSearchAPICompositeTest::overlappingSearch_ignoresStalePrimaryReply_queriesFallbackOnce() {
  DeferredSearchAPI primary{{}}; // both empty → fallback path
  DeferredSearchAPI fallback{{}};

  BookSearchAPIComposite composite{{&primary}};
  composite.setFallbackAPI(&fallback);

  SearchResult cap;
  observeSearch(composite, cap);

  // Two searches dispatched before either primary reply lands (the double-fire
  // seen in the field). The second resets the composite's per-search state.
  composite.search(BookSearchFields{.isbn = 0, .name = u"q"_s, .author = u"q"_s});
  composite.search(BookSearchFields{.isbn = 0, .name = u"q"_s, .author = u"q"_s});
  QCOMPARE(primary.searchCount, 2);

  primary.flushReply(); // first (empty) primary reply → triggers the fallback
  primary.flushReply(); // second (empty) primary reply → must be dropped as stale
  fallback.flushReply();

  // Before the fix, the stale primary reply was consumed as the fallback reply
  // and re-triggered the fallback: two fallback queries and two emissions.
  QVERIFY(cap.received);
  QCOMPARE(cap.count, 1);
  QCOMPARE(fallback.searchCount, 1);
}

void BookSearchAPICompositeTest::fetchDescription_reachesFallback() {
  FakeSearchAPI primary{{}};
  FakeSearchAPI fallback{{}};

  BookSearchAPIComposite composite{{&primary}};
  composite.setFallbackAPI(&fallback);

  composite.fetchDescription(u"gbooks:vol-1"_s);

  QCOMPARE(primary.fetchCount, 1);
  QCOMPARE(fallback.fetchCount, 1);
}

void BookSearchAPICompositeTest::criteria_dropResultsTheSourceDidNotHonour() {
  BookDTO english = makeBook(111);
  english.language = u"en"_s;
  BookDTO russian = makeBook(222);
  russian.language = u"ru"_s;

  FakeSearchAPI primary{{english, russian}};
  BookSearchAPIComposite composite{{&primary}};

  SearchResult cap;
  observeSearch(composite, cap);

  // A catalog can ignore or only partially honour a filter — OpenLibrary's query
  // ranks rather than restricts — so the composite re-checks every result.
  BookSearchFields fields{.isbn = 0, .name = u"anything"_s, .author = u"anything"_s};
  fields.criteria.languages = {u"ru"_s};
  composite.search(fields);

  QVERIFY(cap.received);
  QCOMPARE(cap.books.size(), 1);
  QCOMPARE(cap.books.first().isbn, 222LL);
}

void BookSearchAPICompositeTest::criteria_rejectingEveryPrimaryResult_queriesFallback() {
  BookDTO english = makeBook(111);
  english.language = u"en"_s;
  BookDTO russian = makeBook(333);
  russian.language = u"ru"_s;

  FakeSearchAPI primary{{english}};  // filtered out entirely
  FakeSearchAPI fallback{{russian}}; // survives

  BookSearchAPIComposite composite{{&primary}};
  composite.setFallbackAPI(&fallback);

  SearchResult cap;
  observeSearch(composite, cap);

  BookSearchFields fields{.isbn = 0, .name = u"anything"_s, .author = u"anything"_s};
  fields.criteria.languages = {u"ru"_s};
  composite.search(fields);

  // "Primary returned nothing usable" has to mean the same thing whether the
  // source found nothing or the filter rejected all of it.
  QVERIFY(cap.received);
  QCOMPARE(cap.count, 1);
  QCOMPARE(cap.books.size(), 1);
  QCOMPARE(cap.books.first().isbn, 333LL);
  QCOMPARE(fallback.searchCount, 1);
}

QTEST_GUILESS_MAIN(BookSearchAPICompositeTest)
#include "BookSearchAPICompositeTest.moc"
