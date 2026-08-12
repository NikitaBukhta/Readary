#ifndef LIBRARY_IBOOKNETSEARCH_HPP
#define LIBRARY_IBOOKNETSEARCH_HPP

#include "IBookSearchAPI.hpp"

#include <QHash>
#include <QNetworkAccessManager>

namespace readary::api {

class IBookNetSearchAPI : public IBookSearchAPI {
  Q_OBJECT
public:
  explicit IBookNetSearchAPI(QObject *parent = nullptr);

protected:
  void sendRequest(const QUrl &request);
  static QString asFieldValue(const QString &value);

signals:
  void responseReceived(QNetworkReply *reply);

private:
  void send(const QUrl &url, int attempt);
  void onReplyFinished(QNetworkReply *reply);
  static bool isRetriable(QNetworkReply *reply);

  QNetworkAccessManager _networkManager;
  QHash<QNetworkReply *, int> _attempts;
};

} // namespace readary::api

#endif // LIBRARY_IBOOKNETSEARCH_HPP
