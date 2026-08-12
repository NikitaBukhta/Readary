#ifndef LIBRARY_GOOGLEBOOKSSEARCHAPI_HPP
#define LIBRARY_GOOGLEBOOKSSEARCHAPI_HPP

#include "IBookNetSearchAPI.hpp"

namespace readary::api {

class GoogleBooksSearchAPI : public IBookNetSearchAPI {
  Q_OBJECT
public:
  explicit GoogleBooksSearchAPI(QObject *parent = nullptr);

  void setEndpoint(const QString &endpoint);
  void search(const BookSearchFields &params) override;
  void searchByISBN(qint64 isbn) override;
  void fetchDescription(const QString &workKey) override;

private:
  static QString generateQuery(const BookSearchFields &params);

  void onResponseReceived(QNetworkReply *reply);
  void handleSearchResponse(QNetworkReply *reply);
  void handleVolumeResponse(QNetworkReply *reply);

  QString _endpoint;
};

} // namespace readary::api

#endif // LIBRARY_GOOGLEBOOKSSEARCHAPI_HPP
