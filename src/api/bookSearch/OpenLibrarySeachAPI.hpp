#ifndef LIBRARY_ISBNDBSEACHAPI_H
#define LIBRARY_ISBNDBSEACHAPI_H

#include "IBookNetSearchAPI.hpp"

namespace readary {
namespace api {

class OpenLibrarySeachAPI : public IBookNetSearchAPI{
public:
  OpenLibrarySeachAPI();
  void search(const BookSearchFields& params) override;
  void searchByISBN(qint64 isbn) override;

private slots:
  void onResponseReceived(QNetworkReply *reply);
};

} // api
} // readary

#endif //LIBRARY_ISBNDBSEACHAPI_H
