#ifndef LIBRARY_ISBNDBSEACHAPI_H
#define LIBRARY_ISBNDBSEACHAPI_H

#include "IBookNetSearchAPI.hpp"

namespace readary {
namespace api {

class OpenLibrarySeachAPI : public IBookNetSearchAPI {
public:
  OpenLibrarySeachAPI(QObject *parent = nullptr);
  void search(const BookSearchFields &params) override;
  void searchByISBN(qint64 isbn) override;

private:
  void onResponseReceived(QNetworkReply *reply);
};

} // namespace api
} // namespace readary

#endif // LIBRARY_ISBNDBSEACHAPI_H
