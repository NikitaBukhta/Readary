#include "api/bookSearch/GoogleBooksSearchAPI.hpp"

#include "api/translate/LanguageConverter.hpp"
#include "api/translate/LanguageDetector.hpp"
#include "utils/IsbnValidator.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QUrlQuery>
#include <optional>

using Qt::StringLiterals::operator""_L1;
using Qt::StringLiterals::operator""_s;

namespace {
Q_LOGGING_CATEGORY(lcGoogleBooks, "readary.api.googlebooks")

constexpr int g_pageSize{20};
constexpr auto g_endpoint = "https://www.googleapis.com/books/v1"_L1;
constexpr auto g_volumesPath = "/volumes"_L1;
constexpr auto g_workKeyPrefix = "gbooks:"_L1;
constexpr auto g_insecureScheme = "http://"_L1;
constexpr auto g_secureScheme = "https://"_L1;

QString apiKey() {
#ifdef GOOGLE_BOOKS_API_KEY
  return QStringLiteral(GOOGLE_BOOKS_API_KEY);
#else
  return {};
#endif
}

// Prefer a normalized ISBN-13; fall back to ISBN-10. Returns 0 when neither is
// present or valid so callers can drop the result.
qint64 extractIsbn(const QJsonArray &identifiers) {
  qint64 isbn10 = 0;
  for (const auto &idValue : identifiers) {
    const auto obj = idValue.toObject();
    const auto type = obj.value(u"type"_s).toString();
    const auto normalized = readary::utils::IsbnValidator::convert(obj.value(u"identifier"_s).toString());
    if (!normalized) {
      continue;
    }
    if (type == u"ISBN_13"_s) {
      return *normalized;
    }
    if (type == u"ISBN_10"_s) {
      isbn10 = *normalized;
    }
  }
  return isbn10;
}

// Google's publishedDate is "1965", "1965-06" or "1965-06-01" — the year is
// always the leading four characters.
int extractYear(const QString &publishedDate) { return publishedDate.left(4).toInt(); }

QString asSecureUrl(const QString &url) {
  if (!url.startsWith(g_insecureScheme)) {
    return url;
  }
  return g_secureScheme + url.sliced(g_insecureScheme.size());
}

bool isLatin(const QString &value) {
  return !value.isEmpty() && readary::api::detectScript(value) != readary::api::TextScript::Cyrillic;
}

QStringList extractGenres(const QJsonArray &categories) {
  QStringList genres;
  for (const auto &category : categories) {
    if (const QString name = category.toString().trimmed();
        !name.isEmpty() && !genres.contains(name, Qt::CaseInsensitive)) {
      genres.append(name);
    }
  }
  return genres;
}

std::optional<readary::services::BookDTO> parseVolume(const QJsonObject &item) {
  const auto info = item.value(u"volumeInfo"_s).toObject();
  const qint64 isbn = extractIsbn(info.value(u"industryIdentifiers"_s).toArray());
  if (isbn == 0) {
    return std::nullopt;
  }
  const auto totalPages = info.value(u"pageCount"_s).toInt();
  if (totalPages <= 0) {
    return std::nullopt;
  }

  readary::services::BookDTO book;
  book.isbn = isbn;
  book.workKey = g_workKeyPrefix + item.value(u"id"_s).toString();
  book.name = info.value(u"title"_s).toString();
  book.year = extractYear(info.value(u"publishedDate"_s).toString());
  book.totalPages = totalPages;
  book.publisherName = info.value(u"publisher"_s).toString();
  book.globalRating = info.value(u"averageRating"_s).toDouble();
  book.coverUrl = asSecureUrl(info.value(u"imageLinks"_s).toObject().value(u"thumbnail"_s).toString());
  book.genres = extractGenres(info.value(u"categories"_s).toArray());

  if (const auto authors = info.value(u"authors"_s).toArray(); !authors.isEmpty()) {
    book.authorName = authors.first().toString();
  }

  const QString rawLanguage = info.value(u"language"_s).toString();
  const QString iso = readary::api::LanguageConverter::toIso639_1(rawLanguage);
  book.language = iso.isEmpty() ? rawLanguage : iso;

  return book;
}

} // namespace

namespace readary::api {

GoogleBooksSearchAPI::GoogleBooksSearchAPI(QObject *parent) : IBookNetSearchAPI{parent}, _endpoint{g_endpoint} {
  connect(this, &IBookNetSearchAPI::responseReceived, this, &GoogleBooksSearchAPI::onResponseReceived);
}

void GoogleBooksSearchAPI::setEndpoint(const QString &endpoint) { _endpoint = endpoint; }

QString GoogleBooksSearchAPI::generateQuery(const BookSearchFields &params) {
  if (params.isbn != 0) {
    return u"isbn:"_s + QString::number(params.isbn);
  }

  QStringList terms;
  if (params.name.isEmpty() || params.author.isEmpty() || params.name == params.author) {
    if (const QString text = params.name.isEmpty() ? params.author : params.name; !text.isEmpty()) {
      terms.append(asFieldValue(text));
    }
  } else {
    terms.append(u"intitle:"_s + asFieldValue(params.name) + u" OR inauthor:"_s + asFieldValue(params.author));
  }

  const services::BookFilterCriteria &criteria = params.criteria;
  if (const QString author = criteria.author.trimmed(); isLatin(author)) {
    terms.append(u"inauthor:"_s + asFieldValue(author));
  }
  if (const QString publisher = criteria.publisher.trimmed(); isLatin(publisher)) {
    terms.append(u"inpublisher:"_s + asFieldValue(publisher));
  }
  if (criteria.genres.size() == 1 && isLatin(criteria.genres.first().trimmed())) {
    terms.append(u"subject:"_s + asFieldValue(criteria.genres.first().trimmed()));
  }

  // Criteria alone are a valid search — no typed words required.
  return terms.join(u' ');
}

void GoogleBooksSearchAPI::search(const BookSearchFields &params) {
  const int page = params.page > 0 ? params.page : 1;
  const auto q = generateQuery(params);
  if (q.isEmpty()) {
    qCInfo(lcGoogleBooks) << "no query terms for this source — completing empty";
    emit searchListUpdated({}, false);
    return;
  }

  QUrl url{_endpoint + g_volumesPath};
  QUrlQuery query;
  query.addQueryItem(u"q"_s, q);
  query.addQueryItem(u"startIndex"_s, QString::number((page - 1) * g_pageSize));
  query.addQueryItem(u"maxResults"_s, QString::number(g_pageSize));
  query.addQueryItem(u"printType"_s, u"books"_s);
  if (!params.language.isEmpty()) {
    query.addQueryItem(u"langRestrict"_s, params.language);
  }
  if (const auto key = apiKey(); !key.isEmpty()) {
    query.addQueryItem(u"key"_s, key);
  }
  url.setQuery(query);

  qCInfo(lcGoogleBooks) << "search request (page" << page << "):" << url.toString(QUrl::RemoveQuery) << "q:" << q;
  sendRequest(url);
}

void GoogleBooksSearchAPI::searchByISBN(qint64 isbn) { search(BookSearchFields{.isbn = isbn}); }

void GoogleBooksSearchAPI::fetchDescription(const QString &workKey) {
  if (!workKey.startsWith(g_workKeyPrefix)) {
    return;
  }
  const QString volumeId = workKey.mid(g_workKeyPrefix.size());
  if (volumeId.isEmpty()) {
    return;
  }

  QUrl url{_endpoint + g_volumesPath + u"/"_s + volumeId};
  QUrlQuery query;
  if (const auto key = apiKey(); !key.isEmpty()) {
    query.addQueryItem(u"key"_s, key);
  }
  url.setQuery(query);

  qCInfo(lcGoogleBooks) << "description request:" << url.toString(QUrl::RemoveQuery);
  sendRequest(url);
}

void GoogleBooksSearchAPI::onResponseReceived(QNetworkReply *reply) {
  if (reply == nullptr) {
    qCWarning(lcGoogleBooks) << "reply is not valid";
    return;
  }
  reply->deleteLater();

  if (reply->url().path().endsWith(g_volumesPath)) {
    handleSearchResponse(reply);
  } else {
    handleVolumeResponse(reply);
  }
}

void GoogleBooksSearchAPI::handleSearchResponse(QNetworkReply *reply) {
  const auto httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  if (reply->error()) {
    qCWarning(lcGoogleBooks) << "network error:" << reply->error() << reply->errorString()
                             << "http status:" << httpStatus;
    emit searchListUpdated({}, false);
    return;
  }

  const auto answer = reply->readAll();
  const auto root = QJsonDocument::fromJson(answer).object();
  const auto items = root.value(u"items"_s).toArray();
  qCInfo(lcGoogleBooks) << "response http status:" << httpStatus << "bytes:" << answer.size()
                        << "totalItems:" << root.value(u"totalItems"_s).toInt() << "items returned:" << items.size();

  QList<services::BookDTO> books;
  books.reserve(items.size());
  for (const auto &itemValue : items) {
    if (const auto book = parseVolume(itemValue.toObject())) {
      books.append(*book);
    }
  }

  const bool hasMore = items.size() == g_pageSize;
  qCInfo(lcGoogleBooks) << "kept books:" << books.size()
                        << "skipped (no ISBN or no page count):" << (items.size() - books.size())
                        << "hasMore:" << hasMore;
  emit searchListUpdated(books, hasMore);
}

void GoogleBooksSearchAPI::handleVolumeResponse(QNetworkReply *reply) {
  const QString workKey = g_workKeyPrefix + reply->url().path().section(u'/', -1);

  if (reply->error()) {
    qCWarning(lcGoogleBooks) << "volume fetch error:" << reply->errorString();
    emit descriptionReady(workKey, QString{});
    return;
  }

  const auto obj = QJsonDocument::fromJson(reply->readAll()).object();
  const QString description = obj.value(u"volumeInfo"_s).toObject().value(u"description"_s).toString();
  qCInfo(lcGoogleBooks) << "description for" << workKey << "chars:" << description.size();
  emit descriptionReady(workKey, description);
}

} // namespace readary::api
