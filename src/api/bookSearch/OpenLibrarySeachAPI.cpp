#include "OpenLibrarySeachAPI.hpp"

#include "utils/IsbnValidator.hpp"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringBuilder>
#include <QUrl>

using namespace Qt::StringLiterals;

namespace {
Q_LOGGING_CATEGORY(lcOpenLibrary, "readary.api.openlibrary")

constexpr int g_pageSize{25};
const QString g_apiNameLink{"https://openlibrary.org"};
const QString g_searchLink{g_apiNameLink + "/search.json?q="};
const QString g_fieldsParam{"&fields=title,author_name,first_publish_year,cover_i,isbn,number_of_pages_median"};

QString generateRequest(const readary::api::BookSearchFields &params) {
  const std::string separator = " OR ";

  QString req;
  req.reserve(256); // заранее выделяем память

  bool first = true;

  auto appendField = [&](const QString &field) {
    if (!first) {
      req.append(separator);
    }
    req.append(field);
    first = false;
  };

  if (params.isbn != 0) {
    appendField(QString{"isbn:"} + QString::number(params.isbn));
  }
  if (!params.author.isEmpty()) {
    appendField(QString{"author:"} + params.author);
  }
  if (!params.name.isEmpty()) {
    appendField(QString{"title:"} + params.name);
  }

  return req;
}

} // namespace

namespace readary::api {

OpenLibrarySeachAPI::OpenLibrarySeachAPI(QObject *parent) : IBookNetSearchAPI{parent} {
  connect(this, &IBookNetSearchAPI::responseReceived, this, &OpenLibrarySeachAPI::onResponseReceived);
}
void OpenLibrarySeachAPI::search(const BookSearchFields &params) {
  const int page = params.page > 0 ? params.page : 1;
  const auto paramsRequest = generateRequest(params);
  const auto fullRequest =
      g_searchLink + paramsRequest + g_fieldsParam + u"&limit=%1&page=%2"_s.arg(g_pageSize).arg(page);
  qCInfo(lcOpenLibrary) << "search request (page" << page << "):" << QUrl(fullRequest).toEncoded();
  sendRequest(fullRequest);
}
void OpenLibrarySeachAPI::searchByISBN(qint64 isbn) {
  search(BookSearchFields{.isbn = isbn, .name = {}, .author = {}});
}

void OpenLibrarySeachAPI::onResponseReceived(QNetworkReply *reply) {
  if (reply == nullptr) {
    qCWarning(lcOpenLibrary) << "reply is not valid";
    return;
  }
  reply->deleteLater();

  const auto httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  if (reply->error()) {
    qCWarning(lcOpenLibrary) << "network error:" << reply->error() << reply->errorString()
                             << "http status:" << httpStatus;
    return;
  }

  const auto answer = reply->readAll();
  const auto doc = QJsonDocument::fromJson(answer);
  const auto docs = doc.object().value(u"docs"_s).toArray();
  qCInfo(lcOpenLibrary) << "response http status:" << httpStatus << "bytes:" << answer.size()
                        << "numFound:" << doc.object().value(u"numFound"_s).toInt() << "docs returned:" << docs.size();

  QList<services::BookDTO> books;
  books.reserve(docs.size());
  for (const auto &docValue : docs) {
    const auto obj = docValue.toObject();

    // Normalize to a canonical ISBN-13 so keys match how internal books are stored.
    // Results without a valid ISBN can't become internal books, so they are dropped.
    qint64 isbn = 0;
    const auto isbns = obj.value(u"isbn"_s).toArray();
    for (const auto &isbnValue : isbns) {
      if (const auto normalized = utils::IsbnValidator::convert(isbnValue.toString())) {
        isbn = *normalized;
        break;
      }
    }
    if (isbn == 0) {
      continue;
    }

    const auto totalPages = obj.value(u"number_of_pages_median"_s).toInt();
    if (totalPages <= 0) {
      continue;
    }

    services::BookDTO book;
    book.isbn = isbn;
    book.name = obj.value(u"title"_s).toString();
    book.year = obj.value(u"first_publish_year"_s).toInt();
    book.totalPages = totalPages;

    if (const auto authors = obj.value(u"author_name"_s).toArray(); !authors.isEmpty()) {
      book.authorName = authors.first().toString();
    }
    if (const auto coverId = obj.value(u"cover_i"_s).toInt(); coverId != 0) {
      book.coverUrl = u"https://covers.openlibrary.org/b/id/%1-M.jpg"_s.arg(coverId);
    }

    books.append(book);
  }

  const bool hasMore = docs.size() == g_pageSize;
  qCInfo(lcOpenLibrary) << "kept books:" << books.size()
                        << "skipped (no ISBN or no page count):" << (docs.size() - books.size())
                        << "hasMore:" << hasMore;
  emit searchListUpdated(books, hasMore);
}

} // namespace readary::api
