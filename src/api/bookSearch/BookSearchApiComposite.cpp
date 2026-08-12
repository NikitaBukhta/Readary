#include "BookSearchApiComposite.hpp"

#include "api/translate/LanguageDetector.hpp"

#include <QLoggingCategory>
#include <utility>

using namespace Qt::StringLiterals;

namespace {
Q_LOGGING_CATEGORY(lcComposite, "readary.api.composite")
}

namespace readary::api {

BookSearchAPIComposite::BookSearchAPIComposite(QList<IBookSearchAPI *> &&bookSearchAPIs, QObject *parent)
    : IBookSearchAPI{parent}, _bookSearchAPIs{std::move(bookSearchAPIs)} {
  initConnect();
}

void BookSearchAPIComposite::initConnect() {
  for (const IBookSearchAPI *bookAPI : _bookSearchAPIs) {
    connect(bookAPI, &IBookSearchAPI::searchListUpdated, this, &BookSearchAPIComposite::handlePrimaryResult);
    connect(bookAPI, &IBookSearchAPI::descriptionReady, this, &IBookSearchAPI::descriptionReady);
  }
}

void BookSearchAPIComposite::setFallbackAPI(IBookSearchAPI *fallback) {
  if (_fallbackAPI != nullptr) {
    disconnect(_fallbackAPI, nullptr, this, nullptr);
  }
  _fallbackAPI = fallback;
  if (_fallbackAPI == nullptr) {
    return;
  }

  connect(_fallbackAPI, &IBookSearchAPI::searchListUpdated, this, &BookSearchAPIComposite::handleFallbackResult);
  connect(_fallbackAPI, &IBookSearchAPI::descriptionReady, this, &IBookSearchAPI::descriptionReady);
}

void BookSearchAPIComposite::search(const BookSearchFields &params) {
  BookSearchFields effective = params;
  if (!effective.criteria.languages.isEmpty()) {
    effective.language = effective.criteria.languages.first();
  }
  if (effective.language.isEmpty()) {
    effective.language = detectQueryLanguage(effective.name.isEmpty() ? effective.author : effective.name);
  }
  qCInfo(lcComposite) << "search across" << _bookSearchAPIs.size() << "primary source(s), language filter:"
                      << (effective.language.isEmpty() ? u"none"_s : effective.language);
  beginSearch(effective, false);
}

void BookSearchAPIComposite::searchByISBN(qint64 isbn) {
  qCInfo(lcComposite) << "searchByISBN" << isbn << "across" << _bookSearchAPIs.size() << "primary source(s)";
  beginSearch(BookSearchFields{.isbn = isbn}, true);
}

void BookSearchAPIComposite::beginSearch(const BookSearchFields &params, bool byIsbn) {
  _params = params;
  _byIsbn = byIsbn;
  _aggregated.clear();
  _hasMore = false;
  _pendingPrimary = static_cast<int>(_bookSearchAPIs.size());
  _stage = Stage::AwaitingPrimary;

  if (_pendingPrimary == 0) {
    if (_fallbackAPI == nullptr) {
      finish();
      return;
    }
    _stage = Stage::AwaitingFallback;
    dispatch(_fallbackAPI);
    return;
  }

  for (IBookSearchAPI *bookAPI : _bookSearchAPIs) {
    dispatch(bookAPI);
  }
}

void BookSearchAPIComposite::dispatch(IBookSearchAPI *bookAPI) const {
  if (_byIsbn) {
    bookAPI->searchByISBN(_params.isbn);
  } else {
    bookAPI->search(_params);
  }
}

QList<services::BookDTO> BookSearchAPIComposite::applyCriteria(const QList<services::BookDTO> &books) const {
  if (_params.criteria.isEmpty()) {
    return books;
  }

  QList<services::BookDTO> kept;
  kept.reserve(books.size());
  for (const auto &book : books) {
    if (_params.criteria.matches(book)) {
      kept.append(book);
    }
  }
  qCInfo(lcComposite) << "criteria kept" << kept.size() << "of" << books.size() << "result(s)";
  return kept;
}

void BookSearchAPIComposite::handlePrimaryResult(const QList<services::BookDTO> &rawBooks, bool hasMore) {
  if (_stage != Stage::AwaitingPrimary) {
    qCInfo(lcComposite) << "dropping stale primary reply with" << rawBooks.size() << "book(s)";
    return;
  }

  const QList<services::BookDTO> books = applyCriteria(rawBooks);
  _aggregated.append(books);
  _hasMore = _hasMore || hasMore;

  if (--_pendingPrimary > 0) {
    return;
  }

  if (!_aggregated.isEmpty() || _fallbackAPI == nullptr) {
    finish();
    return;
  }

  qCInfo(lcComposite) << "primary source(s) returned nothing — querying the fallback";
  _stage = Stage::AwaitingFallback;
  dispatch(_fallbackAPI);
}

void BookSearchAPIComposite::handleFallbackResult(const QList<services::BookDTO> &rawBooks, bool hasMore) {
  if (_stage != Stage::AwaitingFallback) {
    qCInfo(lcComposite) << "dropping stale fallback reply with" << rawBooks.size() << "book(s)";
    return;
  }

  _aggregated = applyCriteria(rawBooks);
  _hasMore = hasMore;
  finish();
}

void BookSearchAPIComposite::finish() {
  _stage = Stage::Idle;
  qCInfo(lcComposite) << "search complete —" << _aggregated.size() << "book(s), hasMore:" << _hasMore;
  emit searchListUpdated(_aggregated, _hasMore);
}

void BookSearchAPIComposite::fetchDescription(const QString &workKey) {
  for (IBookSearchAPI *bookAPI : _bookSearchAPIs) {
    bookAPI->fetchDescription(workKey);
  }

  if (_fallbackAPI != nullptr) {
    _fallbackAPI->fetchDescription(workKey);
  }
}

} // namespace readary::api
