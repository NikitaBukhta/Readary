#include "GoogleTranslator.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

using namespace Qt::StringLiterals;

namespace {
Q_LOGGING_CATEGORY(lcGoogleTranslate, "readary.api.googletranslate")

constexpr auto g_endpoint = "https://translate.googleapis.com/translate_a/single"_L1;

QString parseTranslation(const QByteArray &payload) {
  const auto doc = QJsonDocument::fromJson(payload);
  if (!doc.isArray()) {
    return {};
  }
  const auto root = doc.array();
  if (root.isEmpty()) {
    return {};
  }

  QString translated;
  for (const auto &segmentValue : root.first().toArray()) {
    const auto segment = segmentValue.toArray();
    if (!segment.isEmpty()) {
      translated += segment.first().toString();
    }
  }
  return translated;
}

} // namespace

namespace readary::api {

GoogleTranslator::GoogleTranslator(QObject *parent) : ITranslator{parent}, _endpoint{g_endpoint} {
  connect(&_networkManager, &QNetworkAccessManager::finished, this, &GoogleTranslator::onReplyFinished);
}

void GoogleTranslator::setEndpoint(const QString &endpoint) { _endpoint = endpoint; }

void GoogleTranslator::translate(const QString &text, const QString &targetLang, quint64 requestId) {
  if (text.isEmpty() || targetLang.isEmpty()) {
    emit translationReady(requestId, QString{});
    return;
  }

  QUrl url{_endpoint};
  QUrlQuery query;
  query.addQueryItem(u"client"_s, u"gtx"_s);
  query.addQueryItem(u"sl"_s, u"auto"_s);
  query.addQueryItem(u"tl"_s, targetLang);
  query.addQueryItem(u"dt"_s, u"t"_s);
  query.addQueryItem(u"q"_s, text);
  url.setQuery(query);

  qCInfo(lcGoogleTranslate) << "translate request" << requestId << "target:" << targetLang << "chars:" << text.size();

  QNetworkRequest request{url};
  request.setHeader(QNetworkRequest::UserAgentHeader, u"Mozilla/5.0"_s);
  _requests.insert(_networkManager.get(request), requestId);
}

void GoogleTranslator::onReplyFinished(QNetworkReply *reply) {
  if (reply == nullptr) {
    qCWarning(lcGoogleTranslate) << "reply is not valid";
    return;
  }
  reply->deleteLater();

  const auto request = _requests.constFind(reply);
  if (request == _requests.constEnd()) {
    qCWarning(lcGoogleTranslate) << "reply for an unknown request:" << reply->url().toString(QUrl::RemoveQuery);
    return;
  }
  const quint64 requestId = *request;
  _requests.erase(request);

  if (reply->error()) {
    qCWarning(lcGoogleTranslate) << "translate error for request" << requestId << reply->errorString();
    emit translationReady(requestId, QString{});
    return;
  }

  const QString translated = parseTranslation(reply->readAll());
  qCInfo(lcGoogleTranslate) << "translation ready for request" << requestId << "chars:" << translated.size();
  emit translationReady(requestId, translated);
}

} // namespace readary::api
