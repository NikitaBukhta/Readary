#include "services/caching/SearchCache.hpp"
#include "services/dto/BookDTO.hpp"

#include <QCryptographicHash>
#include <QSettings>
#include <QStandardPaths>
#include <QString>
#include <QTest>

using Qt::StringLiterals::operator""_s;

using readary::services::BookDTO;
using readary::services::SearchCache;

namespace {

BookDTO makeBook(qint64 isbn, const QString &name) {
  BookDTO book;
  book.isbn = isbn;
  book.name = name;
  book.authorName = u"Author of %1"_s.arg(name);
  book.year = 1999;
  book.totalPages = 321;
  book.coverUrl = u"https://covers/%1.jpg"_s.arg(isbn);
  book.workKey = u"/works/OL%1W"_s.arg(isbn);
  return book;
}

// Mirrors SearchCache::groupFor so a test can backdate an entry's timestamp.
// White-box coupling, kept intentionally to exercise the TTL branch deterministically.
QString groupFor(const QString &query) {
  const auto hash = QCryptographicHash::hash(query.toUtf8(), QCryptographicHash::Sha1).toHex();
  return u"searchCache/%1"_s.arg(QString::fromLatin1(hash));
}

std::optional<SearchCache::Entry> putThenGetSample() {
  SearchCache::Entry in;
  in.books = {makeBook(111, u"hobbit"_s), makeBook(222, u"lotr"_s)};
  in.nextPage = 3;
  in.hasMore = false;
  SearchCache::put(u"tolkien"_s, in);
  return SearchCache::get(u"tolkien"_s, 7);
}

} // namespace

class SearchCacheTest : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void init();

  void get_missingQuery_returnsNullopt();
  void put_thenGet_roundTripsPaging();
  void put_thenGet_roundTripsBookFields();
  void get_isExactKey_normalizationIsCallersJob();
  void get_staleEntry_returnsNullopt_freshEntry_hits();
  void put_overwritesExistingEntry();
  void roundTrip_withEmptyBookList();
  void distinctQueries_areIndependent();
  void specialCharacterQuery_roundTrips();
};

void SearchCacheTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  QCoreApplication::setOrganizationName("DarieszzBooksTests");
  QCoreApplication::setOrganizationDomain("tests.darieszzbooks.local");
  QCoreApplication::setApplicationName("SearchCacheTest");
}

void SearchCacheTest::init() {
  QSettings settings;
  settings.clear();
  settings.sync();
}

void SearchCacheTest::get_missingQuery_returnsNullopt() {
  QVERIFY(!SearchCache::get(u"never-searched"_s, 7).has_value());
}

void SearchCacheTest::put_thenGet_roundTripsPaging() {
  const auto out = putThenGetSample();
  QVERIFY(out.has_value());
  if (!out.has_value()) {
    return;
  }
  QCOMPARE(out->nextPage, 3);
  QCOMPARE(out->hasMore, false);
  QCOMPARE(out->books.size(), 2);
}

void SearchCacheTest::put_thenGet_roundTripsBookFields() {
  const auto out = putThenGetSample();
  QVERIFY(out.has_value());
  if (!out.has_value()) {
    return;
  }
  QCOMPARE(out->books.at(0).isbn, static_cast<qint64>(111));
  QCOMPARE(out->books.at(0).name, u"hobbit"_s);
  QCOMPARE(out->books.at(0).authorName, u"Author of hobbit"_s);
  QCOMPARE(out->books.at(0).totalPages, 321);
  QCOMPARE(out->books.at(0).workKey, u"/works/OL111W"_s);
  QCOMPARE(out->books.at(1).coverUrl, u"https://covers/222.jpg"_s);
}

void SearchCacheTest::get_isExactKey_normalizationIsCallersJob() {
  SearchCache::put(u"tolkien"_s, {.books = {makeBook(1, u"a"_s)}, .nextPage = 1, .hasMore = true});
  QVERIFY(SearchCache::get(u"tolkien"_s, 7).has_value());
  // SearchCache stores by the exact key; case-folding is the controller's responsibility.
  QVERIFY(!SearchCache::get(u"Tolkien"_s, 7).has_value());
}

void SearchCacheTest::get_staleEntry_returnsNullopt_freshEntry_hits() {
  const QString query = u"aging"_s;
  SearchCache::put(query, {.books = {makeBook(1, u"a"_s)}, .nextPage = 1, .hasMore = true});

  // Backdate the stored timestamp to 100 days ago.
  {
    QSettings settings;
    settings.beginGroup(groupFor(query));
    settings.setValue("updatedAt", QDateTime::currentDateTimeUtc().addDays(-100).toString(Qt::ISODate));
    settings.endGroup();
    settings.sync();
  }

  QVERIFY(!SearchCache::get(query, 7).has_value());  // older than 7 days → miss
  QVERIFY(SearchCache::get(query, 365).has_value()); // within 365 days → hit
}

void SearchCacheTest::put_overwritesExistingEntry() {
  const QString query = u"q"_s;
  SearchCache::put(query, {.books = {makeBook(1, u"a"_s)}, .nextPage = 2, .hasMore = true});
  SearchCache::put(query, {.books = {makeBook(2, u"b"_s), makeBook(3, u"c"_s)}, .nextPage = 5, .hasMore = false});

  const auto out = SearchCache::get(query, 7);
  QVERIFY(out.has_value());
  if (!out.has_value()) {
    return;
  }
  QCOMPARE(out->books.size(), 2);
  QCOMPARE(out->nextPage, 5);
  QCOMPARE(out->hasMore, false);
}

void SearchCacheTest::roundTrip_withEmptyBookList() {
  SearchCache::put(u"empty"_s, {.books = {}, .nextPage = 1, .hasMore = false});

  const auto out = SearchCache::get(u"empty"_s, 7);
  QVERIFY(out.has_value());
  if (!out.has_value()) {
    return;
  }
  QVERIFY(out->books.isEmpty());
  QCOMPARE(out->hasMore, false);
}

void SearchCacheTest::distinctQueries_areIndependent() {
  SearchCache::put(u"a"_s, {.books = {makeBook(1, u"x"_s)}, .nextPage = 1, .hasMore = true});
  SearchCache::put(u"b"_s, {.books = {makeBook(2, u"y"_s), makeBook(3, u"z"_s)}, .nextPage = 1, .hasMore = true});

  const auto a = SearchCache::get(u"a"_s, 7);
  QVERIFY(a.has_value());
  if (!a.has_value()) {
    return;
  }
  QCOMPARE(a->books.size(), 1);

  const auto b = SearchCache::get(u"b"_s, 7);
  QVERIFY(b.has_value());
  if (!b.has_value()) {
    return;
  }
  QCOMPARE(b->books.size(), 2);
}

void SearchCacheTest::specialCharacterQuery_roundTrips() {
  // Keys are hashed, so slashes / spaces / non-latin text must survive as valid storage keys.
  const QString query = QString::fromUtf8("Пушкин / война & мир");
  SearchCache::put(query, {.books = {makeBook(1, u"x"_s)}, .nextPage = 1, .hasMore = true});
  QVERIFY(SearchCache::get(query, 7).has_value());
}

QTEST_GUILESS_MAIN(SearchCacheTest)
#include "SearchCacheTest.moc"
