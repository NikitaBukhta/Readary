#ifndef READARY_API_BOOKSEARCH_OPENLIBRARYSEARCHAPI_HPP
#define READARY_API_BOOKSEARCH_OPENLIBRARYSEARCHAPI_HPP

#include "api/bookSearch/IBookNetSearchAPI.hpp"

namespace readary::api {

class OpenLibrarySearchAPI : public IBookNetSearchAPI {
  Q_OBJECT
public:
  explicit OpenLibrarySearchAPI(QObject *parent = nullptr);

  // Overrides the API base (default: the live OpenLibrary endpoint). For tests.
  void setEndpoint(const QString &endpoint);

  void search(const BookSearchFields &params) override;
  void searchByISBN(qint64 isbn) override;
  void fetchDescription(const QString &workKey) override;

private:
  static QString generateQuery(const BookSearchFields &params);

  void onResponseReceived(QNetworkReply *reply);
  void handleSearchResponse(QNetworkReply *reply);
  void handleWorkResponse(QNetworkReply *reply);

  QString _endpoint;
};

} // namespace readary::api

#endif // READARY_API_BOOKSEARCH_OPENLIBRARYSEARCHAPI_HPP
