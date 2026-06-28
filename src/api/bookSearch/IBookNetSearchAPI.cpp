#include "IBookNetSearchAPI.hpp"

namespace readary {
namespace api {

IBookNetSearchAPI::IBookNetSearchAPI(QObject *parent) : IBookSearchAPI(parent) {
  connect(&_networkManager, &QNetworkAccessManager::finished, this, &IBookNetSearchAPI::responseReceived);
}

QByteArray IBookNetSearchAPI::sendRequest(const QUrl &request){
  const QNetworkRequest netReq(request);
  _networkManager.get(netReq);

  return QByteArray();
}

} // api
}  // readary