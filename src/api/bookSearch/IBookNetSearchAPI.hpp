#ifndef LIBRARY_IBOOKNETSEARCH_HPP
#define LIBRARY_IBOOKNETSEARCH_HPP

#include "IBookSearchAPI.hpp"

#include <QNetworkAccessManager>

namespace readary {
namespace api {

class IBookNetSearchAPI : public IBookSearchAPI {
  Q_OBJECT
public:
  IBookNetSearchAPI(QObject *parent = nullptr);

protected:
  QByteArray sendRequest(const QUrl &request);

signals:
  void responseReceived(QNetworkReply *reply);

private:
  QNetworkAccessManager _networkManager;
};

} // namespace api
} // namespace readary

#endif // LIBRARY_IBOOKNETSEARCH_HPP
