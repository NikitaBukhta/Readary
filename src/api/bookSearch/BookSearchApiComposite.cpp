#include "BookSearchApiComposite.hpp"

#include <QLoggingCategory>
#include <utility>

namespace {
Q_LOGGING_CATEGORY(lcComposite, "readary.api.composite")
}

namespace readary::api {

BookSearchAPIComposite::BookSearchAPIComposite(QList<IBookSearchAPI *> &&bookSearchAPIs, QObject *parent)
    : IBookSearchAPI{parent}, _bookSearchAPIs{std::move(bookSearchAPIs)} {
  initConnect();
}

void BookSearchAPIComposite::search(const BookSearchFields &params) {
  qCInfo(lcComposite) << "fan-out search to" << _bookSearchAPIs.size() << "source(s)";
  _aggregated.clear();
  for (IBookSearchAPI *bookAPI : _bookSearchAPIs) {
    bookAPI->search(params);
  }
}

void BookSearchAPIComposite::searchByISBN(qint64 isbn) {
  qCInfo(lcComposite) << "fan-out searchByISBN" << isbn << "to" << _bookSearchAPIs.size() << "source(s)";
  _aggregated.clear();
  for (IBookSearchAPI *bookAPI : _bookSearchAPIs) {
    bookAPI->searchByISBN(isbn);
  }
}

void BookSearchAPIComposite::fetchDescription(const QString &workKey) {
  for (IBookSearchAPI *bookAPI : _bookSearchAPIs) {
    bookAPI->fetchDescription(workKey);
  }
}

void BookSearchAPIComposite::handleSearchListUpdate(const QList<services::BookDTO> &params, bool hasMore) {
  _aggregated.append(params);
  qCInfo(lcComposite) << "source returned" << params.size() << "book(s); aggregated total:" << _aggregated.size()
                      << "hasMore:" << hasMore;
  emit searchListUpdated(_aggregated, hasMore);
}

void BookSearchAPIComposite::initConnect() {
  for (const IBookSearchAPI *bookAPI : _bookSearchAPIs) {
    connect(bookAPI, &IBookSearchAPI::searchListUpdated, this, &BookSearchAPIComposite::handleSearchListUpdate);
    connect(bookAPI, &IBookSearchAPI::descriptionReady, this, &IBookSearchAPI::descriptionReady);
  }
}

} // namespace readary::api