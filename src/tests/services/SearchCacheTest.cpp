#include "services/SearchCache.hpp"
#include "services/BookDTO.hpp"

#include <QSettings>
#include <QStandardPaths>
#include <QString>
#include <QTest>

using readary::services::BookDTO;
using readary::services::SearchCache;

namespace {

BookDTO makeBook(qint64 isbn, const QString &name) {
  BookDTO book;
  book.isbn = isbn;
  book.name = name;
  book.authorName = QStringLiteral("Author of %1").arg(name);
  book.year = 1999;
  book.totalPages = 321;
  book.coverUrl = QStringLiteral("https://covers/%1.jpg").arg(isbn);
  return book;
}

// Mirrors SearchCache::groupFor so a test can backdate an entry's timestamp.
// White-box coupling, kept intentionally to exercise the TTL branch deterministically.
QString groupFor(const QString &query) {
  const auto hash = QCryptographicHash::hash(query.toUtf8(), QCryptographicHash::Sha1).toHex();
  return QStringLiteral("searchCache/%1").arg(QString::fromLatin1(hash));
}

std::optional<SearchCache::Entry> putThenGetSample() {
  SearchCache::Entry in;
  in.books = {makeBook(111, QStringLiteral("hobbit")), makeBook(222, QStringLiteral("lotr"))};
  in.nextPage = 3;
  in.hasMore = false;
  SearchCache::put(QStringLiteral("tolkien"), in);
  return SearchCache::get(QStringLiteral("tolkien"), 7);
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
  QVERIFY(!SearchCache::get(QStringLiteral("never-searched"), 7).has_value());
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
  QCOMPARE(out->books.at(0).name, QStringLiteral("hobbit"));
  QCOMPARE(out->books.at(0).authorName, QStringLiteral("Author of hobbit"));
  QCOMPARE(out->books.at(0).totalPages, 321);
  QCOMPARE(out->books.at(1).coverUrl, QStringLiteral("https://covers/222.jpg"));
}

void SearchCacheTest::get_isExactKey_normalizationIsCallersJob() {
  SearchCache::put(QStringLiteral("tolkien"),
                   {.books = {makeBook(1, QStringLiteral("a"))}, .nextPage = 1, .hasMore = true});
  QVERIFY(SearchCache::get(QStringLiteral("tolkien"), 7).has_value());
  // SearchCache stores by the exact key; case-folding is the controller's responsibility.
  QVERIFY(!SearchCache::get(QStringLiteral("Tolkien"), 7).has_value());
}

void SearchCacheTest::get_staleEntry_returnsNullopt_freshEntry_hits() {
  const QString query = QStringLiteral("aging");
  SearchCache::put(query, {.books = {makeBook(1, QStringLiteral("a"))}, .nextPage = 1, .hasMore = true});

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
  const QString query = QStringLiteral("q");
  SearchCache::put(query, {.books = {makeBook(1, QStringLiteral("a"))}, .nextPage = 2, .hasMore = true});
  SearchCache::put(
      query,
      {.books = {makeBook(2, QStringLiteral("b")), makeBook(3, QStringLiteral("c"))}, .nextPage = 5, .hasMore = false});

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
  SearchCache::put(QStringLiteral("empty"), {.books = {}, .nextPage = 1, .hasMore = false});

  const auto out = SearchCache::get(QStringLiteral("empty"), 7);
  QVERIFY(out.has_value());
  if (!out.has_value()) {
    return;
  }
  QVERIFY(out->books.isEmpty());
  QCOMPARE(out->hasMore, false);
}

void SearchCacheTest::distinctQueries_areIndependent() {
  SearchCache::put(QStringLiteral("a"), {.books = {makeBook(1, QStringLiteral("x"))}, .nextPage = 1, .hasMore = true});
  SearchCache::put(
      QStringLiteral("b"),
      {.books = {makeBook(2, QStringLiteral("y")), makeBook(3, QStringLiteral("z"))}, .nextPage = 1, .hasMore = true});

  const auto a = SearchCache::get(QStringLiteral("a"), 7);
  QVERIFY(a.has_value());
  if (!a.has_value()) {
    return;
  }
  QCOMPARE(a->books.size(), 1);

  const auto b = SearchCache::get(QStringLiteral("b"), 7);
  QVERIFY(b.has_value());
  if (!b.has_value()) {
    return;
  }
  QCOMPARE(b->books.size(), 2);
}

void SearchCacheTest::specialCharacterQuery_roundTrips() {
  // Keys are hashed, so slashes / spaces / non-latin text must survive as valid storage keys.
  const QString query = QString::fromUtf8("Пушкин / война & мир");
  SearchCache::put(query, {.books = {makeBook(1, QStringLiteral("x"))}, .nextPage = 1, .hasMore = true});
  QVERIFY(SearchCache::get(query, 7).has_value());
}

QTEST_GUILESS_MAIN(SearchCacheTest)
#include "SearchCacheTest.moc"
