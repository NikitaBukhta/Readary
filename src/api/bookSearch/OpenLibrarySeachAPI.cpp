#include "OpenLibrarySeachAPI.hpp"

#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringBuilder>
#include <QUrl>

namespace {
const QString apiNameLink{"https://openlibrary.org"};
const QString searchLink{apiNameLink  + "/search.json?q="};

QString generateRequest(const readary::api::BookSearchFields& params) {
  const std::string separator = " OR ";

  QString req;
  req.reserve(256); // заранее выделяем память

  bool first = true;

  auto appendField = [&](const QString& field) {
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

namespace readary {
namespace api {

 OpenLibrarySeachAPI::OpenLibrarySeachAPI(){

}
void OpenLibrarySeachAPI::search(const BookSearchFields &params){
  const auto paramsRequest = generateRequest(params);
  const auto fullRequest = searchLink + paramsRequest;
  sendRequest(fullRequest);
}

void OpenLibrarySeachAPI::searchByISBN(qint64 isbn){
  Q_UNUSED(isbn);
}

void OpenLibrarySeachAPI::onResponseReceived(QNetworkReply *reply){
   if (reply == nullptr) {
     qWarning() << "reply is not valid";
   }
   if (reply->error()) {
     qWarning() << reply->errorString();
   }

   const auto answer = reply->readAll();
   qDebug() << answer;
}

} // api
} // readary
