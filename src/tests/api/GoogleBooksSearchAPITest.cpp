#include "api/bookSearch/GoogleBooksSearchAPI.hpp"
#include "services/dto/BookDTO.hpp"
#include "support/FakeHttpServer.hpp"
#include "support/SearchCapture.hpp"

#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QString>
#include <QTest>
#include <QUrl>

using Qt::StringLiterals::operator""_s;

using readary::api::BookSearchFields;
using readary::api::GoogleBooksSearchAPI;
using readary::services::BookDTO;
using readary::tests::FakeHttpServer;
using readary::tests::runSearch;
using readary::tests::SearchResult;

namespace {

QByteArray searchBody() {
  const QJsonObject keptInfo{
      {u"title"_s, u"The Kept Book"_s},
      {u"authors"_s, QJsonArray{u"Ada Lovelace"_s, u"Someone Else"_s}},
      {u"publishedDate"_s, u"1998-05-12"_s},
      {u"publisher"_s, u"Acme Press"_s},
      {u"pageCount"_s, 321},
      {u"averageRating"_s, 4.5},
      {u"language"_s, u"en"_s},
      {u"imageLinks"_s, QJsonObject{{u"thumbnail"_s, u"http://books.example/cover.jpg"_s}}},
      {u"industryIdentifiers"_s,
       QJsonArray{QJsonObject{{u"type"_s, u"ISBN_10"_s}, {u"identifier"_s, u"0306406152"_s}},
                  QJsonObject{{u"type"_s, u"ISBN_13"_s}, {u"identifier"_s, u"9780306406157"_s}}}},
  };
  const QJsonObject noIsbnInfo{{u"title"_s, u"No ISBN"_s}, {u"pageCount"_s, 100}};
  const QJsonObject noPagesInfo{
      {u"title"_s, u"No Pages"_s},
      {u"industryIdentifiers"_s,
       QJsonArray{QJsonObject{{u"type"_s, u"ISBN_13"_s}, {u"identifier"_s, u"9780306406157"_s}}}}};

  const QJsonArray items{
      QJsonObject{{u"id"_s, u"vol-kept"_s}, {u"volumeInfo"_s, keptInfo}},
      QJsonObject{{u"id"_s, u"vol-no-isbn"_s}, {u"volumeInfo"_s, noIsbnInfo}},
      QJsonObject{{u"id"_s, u"vol-no-pages"_s}, {u"volumeInfo"_s, noPagesInfo}},
  };
  const QJsonObject root{{u"totalItems"_s, 3}, {u"items"_s, items}};
  return QJsonDocument{root}.toJson(QJsonDocument::Compact);
}

QByteArray volumeBody() {
  const QJsonObject info{{u"title"_s, u"The Kept Book"_s}, {u"description"_s, u"A thorough description."_s}};
  const QJsonObject root{{u"id"_s, u"vol-kept"_s}, {u"volumeInfo"_s, info}};
  return QJsonDocument{root}.toJson(QJsonDocument::Compact);
}

QByteArray respond(const QUrl &target) { return target.path().endsWith(u"/volumes"_s) ? searchBody() : volumeBody(); }

} // namespace

class GoogleBooksSearchAPITest : public QObject {
  Q_OBJECT

private slots:
  void searchKeepsOnlyBooksWithIsbnAndPageCount();
  void searchMapsVolumeFields();
  void distinctNameAndAuthorKeepsFieldTerms();
  void networkErrorEmitsEmptyResult();
  void retriesTransientFailureThenSucceeds();
  void fetchDescriptionReturnsDescriptionForOwnKey();
  void fetchDescriptionIgnoresForeignKey();
  void searchSendsLatinCriteriaOnly();
  void searchWithNothingToAskForCompletesEmpty();
};

void GoogleBooksSearchAPITest::searchKeepsOnlyBooksWithIsbnAndPageCount() {
  FakeHttpServer server{respond};
  QVERIFY(server.start());

  GoogleBooksSearchAPI api;
  api.setEndpoint(server.endpoint(u"/books/v1"_s));

  SearchResult result;
  runSearch(api, BookSearchFields{.isbn = 0, .name = u"war and peace"_s, .author = u"war and peace"_s}, result);
  QTRY_VERIFY_WITH_TIMEOUT(result.received, 5000);

  QCOMPARE(result.books.size(), 1);
  QCOMPARE(result.hasMore, false); // fewer than a full page → no more
  QCOMPARE(server.lastQueryItem(u"q"_s), u"\"war and peace\""_s);
}

void GoogleBooksSearchAPITest::networkErrorEmitsEmptyResult() {
  FakeHttpServer server{respond};
  QVERIFY(server.start());
  server.setFailureStatus("503 Service Unavailable"); // what Google answers when rate limiting
  server.setFailRequests(true);

  GoogleBooksSearchAPI api;
  api.setEndpoint(server.endpoint(u"/books/v1"_s));

  SearchResult result;
  runSearch(api, BookSearchFields{.isbn = 0, .name = u"anything"_s, .author = u"anything"_s}, result);

  QTRY_VERIFY_WITH_TIMEOUT(result.received, 5000);
  QCOMPARE(result.books.size(), 0);
  QCOMPARE(result.hasMore, false);
}

void GoogleBooksSearchAPITest::retriesTransientFailureThenSucceeds() {
  FakeHttpServer server{respond};
  QVERIFY(server.start());
  server.setFailureStatus("503 Service Unavailable");
  server.setTransientFailures(2); // first two 503s, third request serves results

  GoogleBooksSearchAPI api;
  api.setEndpoint(server.endpoint(u"/books/v1"_s));

  SearchResult result;
  runSearch(api, BookSearchFields{.isbn = 0, .name = u"kept"_s, .author = u"kept"_s}, result);

  QTRY_VERIFY_WITH_TIMEOUT(result.received, 5000);
  QCOMPARE(result.books.size(), 1);
  QCOMPARE(server.requestCount(), 3);
}

void GoogleBooksSearchAPITest::searchMapsVolumeFields() {
  FakeHttpServer server{respond};
  QVERIFY(server.start());

  GoogleBooksSearchAPI api;
  api.setEndpoint(server.endpoint(u"/books/v1"_s));

  SearchResult result;
  runSearch(api, BookSearchFields{.isbn = 0, .name = u"kept"_s, .author = u"kept"_s}, result);
  QTRY_VERIFY_WITH_TIMEOUT(result.received, 5000);

  QCOMPARE(result.books.size(), 1);
  const BookDTO &book = result.books.first();
  QCOMPARE(book.isbn, 9780306406157LL);
  QCOMPARE(book.workKey, u"gbooks:vol-kept"_s);
  QCOMPARE(book.name, u"The Kept Book"_s);
  QCOMPARE(book.authorName, u"Ada Lovelace"_s);
  QCOMPARE(book.year, 1998);
  QCOMPARE(book.totalPages, 321);
  QCOMPARE(book.publisherName, u"Acme Press"_s);
  QCOMPARE(book.globalRating, 4.5);
  QCOMPARE(book.language, u"en"_s);
  QCOMPARE(book.coverUrl, u"https://books.example/cover.jpg"_s); // http upgraded to https
  QCOMPARE(server.lastQueryItem(u"q"_s), u"kept"_s);
}

void GoogleBooksSearchAPITest::distinctNameAndAuthorKeepsFieldTerms() {
  FakeHttpServer server{respond};
  QVERIFY(server.start());

  GoogleBooksSearchAPI api;
  api.setEndpoint(server.endpoint(u"/books/v1"_s));

  SearchResult result;
  runSearch(api, BookSearchFields{.isbn = 0, .name = u"dune"_s, .author = u"herbert"_s}, result);
  QTRY_VERIFY_WITH_TIMEOUT(result.received, 5000);
  QCOMPARE(server.lastQueryItem(u"q"_s), u"intitle:dune OR inauthor:herbert"_s);
}

void GoogleBooksSearchAPITest::fetchDescriptionReturnsDescriptionForOwnKey() {
  FakeHttpServer server{respond};
  QVERIFY(server.start());

  GoogleBooksSearchAPI api;
  api.setEndpoint(server.endpoint(u"/books/v1"_s));

  QSignalSpy spy{&api, &GoogleBooksSearchAPI::descriptionReady};
  api.fetchDescription(u"gbooks:vol-kept"_s);
  QVERIFY(spy.wait(5000));

  QCOMPARE(spy.constFirst().at(0).toString(), u"gbooks:vol-kept"_s);
  QCOMPARE(spy.constFirst().at(1).toString(), u"A thorough description."_s);
}

void GoogleBooksSearchAPITest::fetchDescriptionIgnoresForeignKey() {
  FakeHttpServer server{respond};
  QVERIFY(server.start());

  GoogleBooksSearchAPI api;
  api.setEndpoint(server.endpoint(u"/books/v1"_s));

  QSignalSpy spy{&api, &GoogleBooksSearchAPI::descriptionReady};
  api.fetchDescription(u"/works/OL123W"_s); // OpenLibrary key — not ours
  QVERIFY(!spy.wait(1000));
  QCOMPARE(server.requestCount(), 0);
}

void GoogleBooksSearchAPITest::searchSendsLatinCriteriaOnly() {
  FakeHttpServer server{respond};
  QVERIFY(server.start());

  GoogleBooksSearchAPI api;
  api.setEndpoint(server.endpoint(u"/books/v1"_s));

  BookSearchFields fields{.isbn = 0, .name = u"kept"_s, .author = u"kept"_s};
  fields.criteria.author = u"Frank Herbert"_s;
  fields.criteria.publisher = u"Ace"_s;
  fields.criteria.minPages = 100; // no Google syntax for this at all

  SearchResult result;
  runSearch(api, fields, result);
  QTRY_VERIFY_WITH_TIMEOUT(result.received, 5000);

  QVERIFY(server.lastQueryItem(u"q"_s).contains(u"inauthor:\"Frank Herbert\""_s));
  QVERIFY(server.lastQueryItem(u"q"_s).contains(u"inpublisher:Ace"_s));
  QVERIFY(!server.lastQueryItem(u"q"_s).contains(u"pages"_s));

  FakeHttpServer cyrillicServer{respond};
  QVERIFY(cyrillicServer.start());
  GoogleBooksSearchAPI cyrillicApi;
  cyrillicApi.setEndpoint(cyrillicServer.endpoint(u"/books/v1"_s));

  BookSearchFields cyrillicFields{.isbn = 0, .name = u"kept"_s, .author = u"kept"_s};
  cyrillicFields.criteria.author = u"Анджей Сапковский"_s;

  SearchResult cyrillicResult;
  runSearch(cyrillicApi, cyrillicFields, cyrillicResult);
  QTRY_VERIFY_WITH_TIMEOUT(cyrillicResult.received, 5000);
  QVERIFY(!cyrillicServer.lastQueryItem(u"q"_s).contains(u"inauthor:"_s));
}

void GoogleBooksSearchAPITest::searchWithNothingToAskForCompletesEmpty() {
  FakeHttpServer server{respond};
  QVERIFY(server.start());

  GoogleBooksSearchAPI api;
  api.setEndpoint(server.endpoint(u"/books/v1"_s));
  BookSearchFields fields{.isbn = 0, .name = {}, .author = {}, .page = 1, .language = u"ru"_s};
  fields.criteria.languages = {u"ru"_s};

  SearchResult result;
  runSearch(api, fields, result);

  QTRY_VERIFY_WITH_TIMEOUT(result.received, 5000);
  QCOMPARE(result.books.size(), 0);
  QCOMPARE(result.hasMore, false);
  QCOMPARE(server.requestCount(), 0);
}

QTEST_GUILESS_MAIN(GoogleBooksSearchAPITest)
#include "GoogleBooksSearchAPITest.moc"
