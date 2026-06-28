#ifndef LIBRARY_BOOKSEARCHAPIAGREGATOR_HPP
#define LIBRARY_BOOKSEARCHAPIAGREGATOR_HPP

#include "IBookSearchAPI.hpp"
#include <QList>
#include <QMap>

namespace readary {
namespace api {

using Priority = qint8;

class BookSearchAPIComposite : public IBookSearchAPI {
  Q_OBJECT
public:
  BookSearchAPIComposite(QList<IBookSearchAPI *> &&book_search_ap_is, QObject *parent = nullptr);
  void search(const BookSearchFields &params) override;
  void searchByISBN(qint64 isbn) override;

private slots:
  void handleSearchListUpdate(const QList<services::BookDTO> &params);

private:
  void initConnect();

private:
  QList<IBookSearchAPI*> _bookSearchAPIs;
};

} // api
} // readary



#endif //LIBRARY_BOOKSEARCHAPIAGREGATOR_HPP
