#include "OpenLibrarySearchAPI.hpp"

#include "api/translate/LanguageConverter.hpp"
#include "utils/IsbnValidator.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringList>
#include <QUrl>
#include <QUrlQuery>
#include <optional>

using Qt::StringLiterals::operator""_L1;
using Qt::StringLiterals::operator""_s;

namespace {
Q_LOGGING_CATEGORY(lcOpenLibrary, "readary.api.openlibrary")

constexpr int g_pageSize{25};
constexpr auto g_endpoint = "https://openlibrary.org"_L1;
constexpr auto g_searchPath = "/search.json"_L1;
constexpr auto g_jsonSuffix = ".json"_L1;
constexpr auto g_fields =
    "key,title,author_name,first_publish_year,cover_i,isbn,number_of_pages_median,language,publisher,subject"_L1;
constexpr int g_maxGenres{5};
constexpr auto g_workKeyPrefix = "/works/"_L1;

qint64 extractIsbn(const QJsonArray &isbns) {
  for (const auto &isbnValue : isbns) {
    if (const auto normalized = readary::utils::IsbnValidator::convert(isbnValue.toString())) {
      return *normalized;
    }
  }
  return 0;
}

QStringList extractGenres(const QJsonArray &subjects) {
  QStringList genres;
  for (const auto &subject : subjects) {
    if (genres.size() >= g_maxGenres) {
      break;
    }
    if (const QString name = subject.toString().trimmed();
        !name.isEmpty() && !genres.contains(name, Qt::CaseInsensitive)) {
      genres.append(name);
    }
  }
  return genres;
}

QString extractLanguages(const QJsonArray &codes) {
  QStringList languages;
  for (const auto &code : codes) {
    if (const QString iso = readary::api::LanguageConverter::toIso639_1(code.toString());
        !iso.isEmpty() && !languages.contains(iso)) {
      languages.append(iso);
    }
  }
  return languages.join(u", "_s);
}

std::optional<readary::services::BookDTO> parseDoc(const QJsonObject &obj) {
  const qint64 isbn = extractIsbn(obj.value(u"isbn"_s).toArray());
  if (isbn == 0) {
    return std::nullopt;
  }
  const auto totalPages = obj.value(u"number_of_pages_median"_s).toInt();
  if (totalPages <= 0) {
    return std::nullopt;
  }

  readary::services::BookDTO book;
  book.isbn = isbn;
  book.workKey = obj.value(u"key"_s).toString();
  book.name = obj.value(u"title"_s).toString();
  book.year = obj.value(u"first_publish_year"_s).toInt();
  book.totalPages = totalPages;
  book.genres = extractGenres(obj.value(u"subject"_s).toArray());
  book.language = extractLanguages(obj.value(u"language"_s).toArray());

  if (const auto authors = obj.value(u"author_name"_s).toArray(); !authors.isEmpty()) {
    book.authorName = authors.first().toString();
  }
  if (const auto publishers = obj.value(u"publisher"_s).toArray(); !publishers.isEmpty()) {
    book.publisherName = publishers.first().toString();
  }
  if (const auto coverId = obj.value(u"cover_i"_s).toInt(); coverId != 0) {
    book.coverUrl = u"https://covers.openlibrary.org/b/id/%1-M.jpg"_s.arg(coverId);
  }
  return book;
}

} // namespace

namespace readary::api {

OpenLibrarySearchAPI::OpenLibrarySearchAPI(QObject *parent) : IBookNetSearchAPI{parent}, _endpoint{g_endpoint} {
  connect(this, &IBookNetSearchAPI::responseReceived, this, &OpenLibrarySearchAPI::onResponseReceived);
}

void OpenLibrarySearchAPI::setEndpoint(const QString &endpoint) { _endpoint = endpoint; }

QString OpenLibrarySearchAPI::generateQuery(const BookSearchFields &params) {
  QStringList terms;
  if (params.isbn != 0) {
    terms.append(u"isbn:"_s + QString::number(params.isbn));
  }
  if (!params.author.isEmpty()) {
    terms.append(u"author:"_s + asFieldValue(params.author));
  }
  if (!params.name.isEmpty()) {
    terms.append(u"title:"_s + asFieldValue(params.name));
  }
  QStringList clauses;
  if (!terms.isEmpty()) {
    clauses.append(terms.size() > 1 ? u"("_s + terms.join(u" OR "_s) + u")"_s : terms.first());
  }
  if (const QString marc = LanguageConverter::toMarc(params.language); !marc.isEmpty()) {
    clauses.append(u"language:"_s + marc);
  }

  const services::BookFilterCriteria &criteria = params.criteria;
  if (const QString author = criteria.author.trimmed(); !author.isEmpty()) {
    clauses.append(u"author:"_s + asFieldValue(author));
  }
  if (const QString publisher = criteria.publisher.trimmed(); !publisher.isEmpty()) {
    clauses.append(u"publisher:"_s + asFieldValue(publisher));
  }
  if (!criteria.genres.isEmpty()) {
    QStringList subjects;
    subjects.reserve(criteria.genres.size());
    for (const QString &genre : criteria.genres) {
      subjects.append(u"subject:"_s + asFieldValue(genre));
    }
    clauses.append(u"("_s + subjects.join(u" OR "_s) + u")"_s);
  }

  return clauses.join(u" AND "_s);
}

void OpenLibrarySearchAPI::search(const BookSearchFields &params) {
  const int page = params.page > 0 ? params.page : 1;
  const QString q = generateQuery(params);
  if (q.isEmpty()) {
    qCInfo(lcOpenLibrary) << "no query terms for this source — completing empty";
    emit searchListUpdated({}, false);
    return;
  }

  QUrl url{_endpoint + g_searchPath};
  QUrlQuery query;
  query.addQueryItem(u"q"_s, q);
  query.addQueryItem(u"fields"_s, g_fields);
  query.addQueryItem(u"limit"_s, QString::number(g_pageSize));
  query.addQueryItem(u"page"_s, QString::number(page));
  url.setQuery(query);

  qCInfo(lcOpenLibrary) << "search request (page" << page << "):" << url.toString(QUrl::RemoveQuery) << "q:" << q;
  sendRequest(url);
}

void OpenLibrarySearchAPI::searchByISBN(qint64 isbn) { search(BookSearchFields{.isbn = isbn}); }

void OpenLibrarySearchAPI::fetchDescription(const QString &workKey) {
  if (!workKey.startsWith(g_workKeyPrefix)) {
    return;
  }

  const QUrl url{_endpoint + workKey + g_jsonSuffix};
  qCInfo(lcOpenLibrary) << "description request:" << url.toString();
  sendRequest(url);
}

void OpenLibrarySearchAPI::onResponseReceived(QNetworkReply *reply) {
  if (reply == nullptr) {
    qCWarning(lcOpenLibrary) << "reply is not valid";
    return;
  }
  reply->deleteLater();

  if (reply->url().path().startsWith(g_workKeyPrefix)) {
    handleWorkResponse(reply);
  } else {
    handleSearchResponse(reply);
  }
}

void OpenLibrarySearchAPI::handleSearchResponse(QNetworkReply *reply) {
  const auto httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  if (reply->error()) {
    qCWarning(lcOpenLibrary) << "network error:" << reply->error() << reply->errorString()
                             << "http status:" << httpStatus;
    emit searchListUpdated({}, false);
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
    if (const auto book = parseDoc(docValue.toObject())) {
      books.append(*book);
    }
  }

  const bool hasMore = docs.size() == g_pageSize;
  qCInfo(lcOpenLibrary) << "kept books:" << books.size()
                        << "skipped (no ISBN or no page count):" << (docs.size() - books.size())
                        << "hasMore:" << hasMore;
  emit searchListUpdated(books, hasMore);
}

void OpenLibrarySearchAPI::handleWorkResponse(QNetworkReply *reply) {
  QString workKey = reply->url().path();
  if (workKey.endsWith(g_jsonSuffix)) {
    workKey.chop(g_jsonSuffix.size());
  }

  if (reply->error()) {
    qCWarning(lcOpenLibrary) << "work fetch error:" << reply->errorString();
    emit descriptionReady(workKey, QString{});
    return;
  }

  const auto obj = QJsonDocument::fromJson(reply->readAll()).object();
  const auto descriptionValue = obj.value(u"description"_s);
  QString description;
  if (descriptionValue.isString()) {
    description = descriptionValue.toString();
  } else if (descriptionValue.isObject()) {
    description = descriptionValue.toObject().value(u"value"_s).toString();
  }

  qCInfo(lcOpenLibrary) << "description for" << workKey << "chars:" << description.size();
  emit descriptionReady(workKey, description);
}

} // namespace readary::api
