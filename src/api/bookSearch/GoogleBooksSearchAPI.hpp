#ifndef READARY_API_BOOKSEARCH_GOOGLEBOOKSSEARCHAPI_HPP
#define READARY_API_BOOKSEARCH_GOOGLEBOOKSSEARCHAPI_HPP

#include "api/bookSearch/IBookNetSearchAPI.hpp"

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

#endif // READARY_API_BOOKSEARCH_GOOGLEBOOKSSEARCHAPI_HPP
