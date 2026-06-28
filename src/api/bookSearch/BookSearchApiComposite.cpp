#include "BookSearchApiComposite.hpp"

namespace readary {
namespace api {



BookSearchAPIComposite::BookSearchAPIComposite(QList<IBookSearchAPI *> &&book_search_ap_is, QObject *parent) : IBookSearchAPI(parent) {
  initConnect();
}

void BookSearchAPIComposite::search(const BookSearchFields &params){
  for (IBookSearchAPI *bookAPI : _bookSearchAPIs) {
    bookAPI->search(params);
  }
}

void BookSearchAPIComposite::searchByISBN(qint64 isbn){
  for (IBookSearchAPI *bookAPI : _bookSearchAPIs) {
    bookAPI->searchByISBN(isbn);
  }
}

void BookSearchAPIComposite::handleSearchListUpdate(const QList<services::BookDTO> &params){
}

void BookSearchAPIComposite::initConnect(){
  for (IBookSearchAPI *bookAPI : _bookSearchAPIs) {
    connect(bookAPI, &IBookSearchAPI::searchListUpdated, this, &BookSearchAPIComposite::handleSearchListUpdate);
  }
}

} // api
} // readary