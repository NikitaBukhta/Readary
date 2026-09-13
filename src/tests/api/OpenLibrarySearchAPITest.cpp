#include "api/bookSearch/OpenLibrarySearchAPI.hpp"
#include "services/dto/BookDTO.hpp"
#include "support/FakeHttpServer.hpp"
#include "support/SearchCapture.hpp"

#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QTest>

using Qt::StringLiterals::operator""_s;

using readary::api::BookSearchFields;
using readary::api::OpenLibrarySearchAPI;
using readary::services::BookDTO;
using readary::tests::FakeHttpServer;
using readary::tests::runSearch;
using readary::tests::SearchResult;

namespace {

QByteArray respond(const QUrl &target) {
  Q_UNUSED(target)

  const QJsonObject kept{
      {u"key"_s, u"/works/OL1W"_s},
      {u"title"_s, u"The Kept Book"_s},
      {u"author_name"_s, QJsonArray{u"Ada Lovelace"_s}},
      {u"first_publish_year"_s, 1998},
      {u"cover_i"_s, 42},
      {u"number_of_pages_median"_s, 321},
      {u"isbn"_s, QJsonArray{u"9780306406157"_s}},
      {u"language"_s, QJsonArray{u"eng"_s}},
  };
  const QJsonObject noIsbn{
      {u"key"_s, u"/works/OL2W"_s}, {u"title"_s, u"No ISBN"_s}, {u"number_of_pages_median"_s, 100}};
  const QJsonObject noPages{
      {u"key"_s, u"/works/OL3W"_s}, {u"title"_s, u"No Pages"_s}, {u"isbn"_s, QJsonArray{u"9780306406157"_s}}};

  const QJsonObject root{{u"numFound"_s, 3}, {u"docs"_s, QJsonArray{kept, noIsbn, noPages}}};
  return QJsonDocument{root}.toJson(QJsonDocument::Compact);
}

} // namespace

class OpenLibrarySearchAPITest : public QObject {
  Q_OBJECT

private slots:
  void searchQuotesMultiWordFields();
  void searchLeavesSingleWordFieldBare();
  void searchKeepsOnlyBooksWithIsbnAndPageCount();
  void networkErrorEmitsEmptyResult();
  void searchSendsCategoricalCriteriaButNotRanges();
  void searchWithCriteriaButNoTextStillQueries();
};

void OpenLibrarySearchAPITest::searchQuotesMultiWordFields() {
  FakeHttpServer server{respond};
  QVERIFY(server.start());

  OpenLibrarySearchAPI api;
  api.setEndpoint(server.endpoint());

  SearchResult result;
  runSearch(api, BookSearchFields{.isbn = 0, .name = u"war and peace"_s, .author = u"war and peace"_s}, result);
  QTRY_VERIFY_WITH_TIMEOUT(result.received, 5000);
  QVERIFY(server.lastQueryItem(u"q"_s).contains(u"title:\"war and peace\""_s));
  QVERIFY(server.lastQueryItem(u"q"_s).contains(u"author:\"war and peace\""_s));
}

void OpenLibrarySearchAPITest::searchLeavesSingleWordFieldBare() {
  FakeHttpServer server{respond};
  QVERIFY(server.start());

  OpenLibrarySearchAPI api;
  api.setEndpoint(server.endpoint());

  SearchResult result;
  runSearch(api, BookSearchFields{.isbn = 0, .name = u"kept"_s, .author = u"kept"_s}, result);
  QTRY_VERIFY_WITH_TIMEOUT(result.received, 5000);

  QVERIFY(server.lastQueryItem(u"q"_s).contains(u"title:kept"_s));
  QVERIFY(!server.lastQueryItem(u"q"_s).contains(u"title:\"kept\""_s));
}

void OpenLibrarySearchAPITest::searchKeepsOnlyBooksWithIsbnAndPageCount() {
  FakeHttpServer server{respond};
  QVERIFY(server.start());

  OpenLibrarySearchAPI api;
  api.setEndpoint(server.endpoint());

  SearchResult result;
  runSearch(api, BookSearchFields{.isbn = 0, .name = u"kept"_s, .author = u"kept"_s}, result);
  QTRY_VERIFY_WITH_TIMEOUT(result.received, 5000);

  QCOMPARE(result.books.size(), 1);
  const BookDTO &book = result.books.first();
  QCOMPARE(book.isbn, 9780306406157LL);
  QCOMPARE(book.workKey, u"/works/OL1W"_s);
  QCOMPARE(book.name, u"The Kept Book"_s);
  QCOMPARE(book.authorName, u"Ada Lovelace"_s);
  QCOMPARE(book.year, 1998);
  QCOMPARE(book.totalPages, 321);
}

void OpenLibrarySearchAPITest::networkErrorEmitsEmptyResult() {
  FakeHttpServer server{respond};
  QVERIFY(server.start());
  server.setFailRequests(true);

  OpenLibrarySearchAPI api;
  api.setEndpoint(server.endpoint());

  SearchResult result;
  runSearch(api, BookSearchFields{.isbn = 0, .name = u"anything"_s, .author = u"anything"_s}, result);

  QTRY_VERIFY_WITH_TIMEOUT(result.received, 5000);
  QCOMPARE(result.books.size(), 0);
  QCOMPARE(result.hasMore, false);
}

void OpenLibrarySearchAPITest::searchSendsCategoricalCriteriaButNotRanges() {
  FakeHttpServer server{respond};
  QVERIFY(server.start());

  OpenLibrarySearchAPI api;
  api.setEndpoint(server.endpoint());

  BookSearchFields fields{.isbn = 0, .name = u"dune"_s, .author = u"dune"_s};
  fields.criteria.author = u"Frank Herbert"_s;
  fields.criteria.publisher = u"Ace Books"_s;
  fields.criteria.genres = {u"Science Fiction"_s, u"Fiction"_s};
  fields.criteria.minPages = 100;
  fields.criteria.maxPages = 300;
  fields.criteria.minYear = 1960;

  SearchResult result;
  runSearch(api, fields, result);
  QTRY_VERIFY_WITH_TIMEOUT(result.received, 5000);

  const QString sent = server.lastQueryItem(u"q"_s);
  QVERIFY(sent.contains(u"AND author:\"Frank Herbert\""_s));
  QVERIFY(sent.contains(u"AND publisher:\"Ace Books\""_s));
  QVERIFY(sent.contains(u"AND (subject:\"Science Fiction\" OR subject:Fiction)"_s));
  QVERIFY(!sent.contains(u"number_of_pages_median"_s));
  QVERIFY(!sent.contains(u"first_publish_year"_s));
}

void OpenLibrarySearchAPITest::searchWithCriteriaButNoTextStillQueries() {
  FakeHttpServer server{respond};
  QVERIFY(server.start());

  OpenLibrarySearchAPI api;
  api.setEndpoint(server.endpoint());

  BookSearchFields fields{.isbn = 0, .name = {}, .author = {}};
  fields.criteria.author = u"Herbert"_s;
  fields.criteria.genres = {u"Science Fiction"_s};

  SearchResult result;
  runSearch(api, fields, result);
  QTRY_VERIFY_WITH_TIMEOUT(result.received, 5000);

  const QString sent = server.lastQueryItem(u"q"_s);
  QCOMPARE(sent, u"author:Herbert AND (subject:\"Science Fiction\")"_s);
  QVERIFY(!sent.startsWith(u" AND "_s)); // no empty leading clause
}

QTEST_GUILESS_MAIN(OpenLibrarySearchAPITest)
#include "OpenLibrarySearchAPITest.moc"
