#include "SearchCache.hpp"

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QSettings>

using namespace Qt::StringLiterals;

namespace {
Q_LOGGING_CATEGORY(lcSearchCache, "readary.services.searchCache")

QString serialize(const QList<readary::services::BookDTO> &books) {
  QJsonArray arr;
  for (const auto &book : books) {
    arr.append(QJsonObject::fromVariantMap(book.toMap()));
  }
  return QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

QList<readary::services::BookDTO> deserialize(const QString &json) {
  QList<readary::services::BookDTO> books;
  const auto doc = QJsonDocument::fromJson(json.toUtf8());
  const auto arr = doc.array();
  books.reserve(arr.size());
  for (const auto &value : arr) {
    books.append(readary::services::BookDTO::fromMap(value.toObject().toVariantMap()));
  }
  return books;
}

} // namespace

namespace readary::services {

QString SearchCache::groupFor(const QString &query) {
  const auto hash = QCryptographicHash::hash(query.toUtf8(), QCryptographicHash::Sha1).toHex();
  return u"searchCache/%1"_s.arg(QString::fromLatin1(hash));
}

std::optional<SearchCache::Entry> SearchCache::get(const QString &query, int maxAgeDays) {
  QSettings settings;
  settings.beginGroup(groupFor(query));

  if (!settings.contains("books")) {
    settings.endGroup();
    return std::nullopt;
  }

  const QDateTime updatedAt = QDateTime::fromString(settings.value("updatedAt").toString(), Qt::ISODate);
  const QString booksJson = settings.value("books").toString();
  Entry entry;
  entry.nextPage = settings.value("nextPage", 1).toInt();
  entry.hasMore = settings.value("hasMore", true).toBool();
  settings.endGroup();

  if (!updatedAt.isValid() || updatedAt.daysTo(QDateTime::currentDateTimeUtc()) > maxAgeDays) {
    qCInfo(lcSearchCache) << "cache stale/invalid for query:" << query;
    return std::nullopt;
  }

  entry.books = deserialize(booksJson);
  qCInfo(lcSearchCache) << "cache read for query:" << query << "books:" << entry.books.size();
  return entry;
}

void SearchCache::put(const QString &query, const Entry &entry) {
  QSettings settings;
  settings.beginGroup(groupFor(query));
  settings.setValue("books", serialize(entry.books));
  settings.setValue("nextPage", entry.nextPage);
  settings.setValue("hasMore", entry.hasMore);
  settings.setValue("updatedAt", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
  settings.endGroup();
  settings.sync();
  qCInfo(lcSearchCache) << "cache write for query:" << query << "books:" << entry.books.size();
}

} // namespace readary::services
