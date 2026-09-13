#include "api/bookSearch/IBookNetSearchAPI.hpp"

#include <QLoggingCategory>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

using Qt::StringLiterals::operator""_s;

namespace {
Q_LOGGING_CATEGORY(lcBookNet, "readary.api.net")

constexpr int g_maxAttempts{3};
constexpr int g_retryBackoffMs{200};
constexpr int g_httpTooManyRequests{429};
constexpr int g_httpInternalError{500};
constexpr int g_httpBadGateway{502};
constexpr int g_httpUnavailable{503};
constexpr int g_httpGatewayTimeout{504};

} // namespace

namespace readary::api {

IBookNetSearchAPI::IBookNetSearchAPI(QObject *parent) : IBookSearchAPI{parent} {
  connect(&_networkManager, &QNetworkAccessManager::finished, this, &IBookNetSearchAPI::onReplyFinished);
}

void IBookNetSearchAPI::sendRequest(const QUrl &request) { send(request, 1); }

QString IBookNetSearchAPI::asFieldValue(const QString &value) {
  if (value.contains(u' ')) {
    return u"\""_s + value + u"\""_s;
  }
  return value;
}

void IBookNetSearchAPI::send(const QUrl &url, int attempt) {
  const QNetworkRequest netReq{url};
  _attempts.insert(_networkManager.get(netReq), attempt);
}

void IBookNetSearchAPI::onReplyFinished(QNetworkReply *reply) {
  if (reply == nullptr) {
    qCWarning(lcBookNet) << "reply is not valid";
    return;
  }

  const int attempt = _attempts.take(reply);
  if (attempt > 0 && attempt < g_maxAttempts && isRetriable(reply)) {
    const QUrl url = reply->request().url();
    qCInfo(lcBookNet) << "retrying attempt" << attempt + 1 << "of" << g_maxAttempts << "after" << reply->errorString()
                      << "for" << url.toString(QUrl::RemoveQuery);
    reply->deleteLater();
    QTimer::singleShot(g_retryBackoffMs * attempt, this, [this, url, attempt] { send(url, attempt + 1); });
    return;
  }

  emit responseReceived(reply);
}

bool IBookNetSearchAPI::isRetriable(QNetworkReply *reply) {
  switch (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()) {
  case g_httpTooManyRequests:
  case g_httpInternalError:
  case g_httpBadGateway:
  case g_httpUnavailable:
  case g_httpGatewayTimeout:
    return true;
  default:
    break;
  }

  switch (reply->error()) {
  case QNetworkReply::TimeoutError:
  case QNetworkReply::TemporaryNetworkFailureError:
  case QNetworkReply::ProxyTimeoutError:
    return true;
  default:
    return false;
  }
}

} // namespace readary::api
