#ifndef LIBRARY_BOOKSEARCHAPIAGREGATOR_HPP
#define LIBRARY_BOOKSEARCHAPIAGREGATOR_HPP

#include "IBookSearchAPI.hpp"
#include <QList>

namespace readary {
namespace api {

using Priority = qint8;

class BookSearchAPIComposite : public IBookSearchAPI {
  Q_OBJECT
public:
  BookSearchAPIComposite(QList<IBookSearchAPI *> &&bookSearchAPIs, QObject *parent = nullptr);
  void search(const BookSearchFields &params) override;
  void searchByISBN(qint64 isbn) override;

private slots:
  void handleSearchListUpdate(const QList<services::BookDTO> &params, bool hasMore);

private:
  void initConnect();

private:
  QList<IBookSearchAPI *> _bookSearchAPIs;
  QList<services::BookDTO> _aggregated;
};

} // namespace api
} // namespace readary

#endif // LIBRARY_BOOKSEARCHAPIAGREGATOR_HPP
