#include "GlobalBookSearchController.hpp"

#include "services/SearchCache.hpp"
#include "utils/IsbnValidator.hpp"

#include <QLoggingCategory>

namespace {
Q_LOGGING_CATEGORY(lcGlobalSearch, "readary.controllers.globalsearch")
constexpr int g_searchCacheMaxAgeDays{7};
} // namespace

namespace readary::controllers {

GlobalBookSearchController *GlobalBookSearchController::s_instance = nullptr;

GlobalBookSearchController::GlobalBookSearchController(QObject *parent)
    : QObject{parent}, _bookSearchAPI{nullptr}, _resultsModel{new models::GlobalBookSearchListModel{this}} {}

void GlobalBookSearchController::setBookSearchAPI(api::IBookSearchAPI *api) {
  _bookSearchAPI = api;
  if (_bookSearchAPI != nullptr) {
    connect(_bookSearchAPI, &api::IBookSearchAPI::searchListUpdated, this,
            &GlobalBookSearchController::onSearchResults);
    connect(_bookSearchAPI, &api::IBookSearchAPI::descriptionReady, this,
            &GlobalBookSearchController::onDescriptionReady);
  }
}

models::GlobalBookSearchListModel *GlobalBookSearchController::resultsModel() const { return _resultsModel; }

void GlobalBookSearchController::search(const QString &query) {
  const QString key = normalizeKey(query);
  if (key.isEmpty()) {
    return;
  }
  qCInfo(lcGlobalSearch) << "search query:" << key;
  _activeQuery = key;

  if (tryServeFromMemoryCache(key) || tryServeFromDiskCache(key)) {
    return;
  }
  startFreshSearch(key);
}

QString GlobalBookSearchController::normalizeKey(const QString &query) { return query.trimmed().toLower(); }

bool GlobalBookSearchController::tryServeFromMemoryCache(const QString &key) {
  const auto cached = _searchCache.constFind(key);
  if (cached == _searchCache.constEnd()) {
    return false;
  }
  qCInfo(lcGlobalSearch) << "memory cache hit — returning" << cached->books.size()
                         << "cached result(s), hasMore:" << cached->hasMore;
  _resultsModel->setBooks(cached->books);
  return true;
}

bool GlobalBookSearchController::tryServeFromDiskCache(const QString &key) {
  const auto persisted = services::SearchCache::get(key, g_searchCacheMaxAgeDays);
  if (!persisted) {
    return false;
  }
  qCInfo(lcGlobalSearch) << "disk cache hit — returning" << persisted->books.size()
                         << "cached result(s), hasMore:" << persisted->hasMore;
  _searchCache.insert(
      key, CachedSearch{.books = persisted->books, .nextPage = persisted->nextPage, .hasMore = persisted->hasMore});
  _resultsModel->setBooks(persisted->books);
  return true;
}

void GlobalBookSearchController::startFreshSearch(const QString &key) {
  _searchCache.insert(key, CachedSearch{});
  _resultsModel->setBooks({});
  requestPage(key, 1);
}

void GlobalBookSearchController::loadMore() {
  if (_activeQuery.isEmpty() || _loading) {
    return;
  }
  const auto it = _searchCache.constFind(_activeQuery);
  if (it == _searchCache.constEnd() || !it->hasMore) {
    return;
  }
  qCInfo(lcGlobalSearch) << "loadMore — query:" << _activeQuery << "page:" << it->nextPage;
  requestPage(_activeQuery, it->nextPage);
}

void GlobalBookSearchController::requestPage(const QString &query, int page) {
  if (_bookSearchAPI == nullptr) {
    qCWarning(lcGlobalSearch) << "search aborted — book search API not set";
    return;
  }

  _loading = true;
  _pendingQuery = query;
  _pendingPage = page;

  if (const auto isbn = queryAsIsbn(query)) {
    qCInfo(lcGlobalSearch) << "query recognized as ISBN:" << *isbn;
    _bookSearchAPI->searchByISBN(*isbn);
    return;
  }

  const api::BookSearchFields searchFields{
      .isbn = 0,
      .name = query,
      .author = query,
      .page = page,
  };
  _bookSearchAPI->search(searchFields);
}

std::optional<qint64> GlobalBookSearchController::queryAsIsbn(const QString &query) {
  // toUpper so a lowercased ISBN-10 check digit ('x') is still recognized.
  return utils::IsbnValidator::convert(query.toUpper());
}

void GlobalBookSearchController::onSearchResults(const QList<services::BookDTO> &books, bool hasMore) {
  _loading = false;
  if (_pendingQuery.isEmpty()) {
    return;
  }

  const CachedSearch &entry = accumulatePage(_pendingQuery, _pendingPage, books, hasMore);
  services::SearchCache::put(_pendingQuery,
                             {.books = entry.books, .nextPage = entry.nextPage, .hasMore = entry.hasMore});

  if (_pendingQuery == _activeQuery) {
    showPage(books, _pendingPage <= 1);
  }
}

GlobalBookSearchController::CachedSearch &
GlobalBookSearchController::accumulatePage(const QString &query, int page, const QList<services::BookDTO> &books,
                                           bool hasMore) {
  auto it = _searchCache.find(query);
  if (it == _searchCache.end()) {
    it = _searchCache.insert(query, CachedSearch{});
  }
  CachedSearch &entry = *it;
  if (page <= 1) {
    entry.books = books;
  } else {
    entry.books.append(books);
  }
  entry.nextPage = page + 1;
  entry.hasMore = hasMore;
  return entry;
}

void GlobalBookSearchController::showPage(const QList<services::BookDTO> &books, bool firstPage) {
  if (firstPage) {
    _resultsModel->setBooks(books);
  } else {
    _resultsModel->appendBooks(books);
  }
}

void GlobalBookSearchController::openBook(qint64 isbn) {
  if (isbn <= 0) {
    return;
  }
  const auto book = _resultsModel->getBook(isbn);
  if (book.isbn <= 0) {
    qCWarning(lcGlobalSearch) << "openBook — isbn not in results:" << isbn;
    return;
  }

  _pendingImport = book;
  if (_bookSearchAPI != nullptr && !book.workKey.isEmpty()) {
    qCInfo(lcGlobalSearch) << "fetching description before import — isbn:" << book.isbn << "workKey:" << book.workKey;
    _bookSearchAPI->fetchDescription(book.workKey);
    return;
  }

  qCInfo(lcGlobalSearch) << "import & open requested — isbn:" << book.isbn << "name:" << book.name;
  emit bookImportRequested(book);
}

void GlobalBookSearchController::onDescriptionReady(const QString &workKey, const QString &description) {
  if (workKey != _pendingImport.workKey) {
    return;
  }
  _pendingImport.description = description;
  qCInfo(lcGlobalSearch) << "import & open requested — isbn:" << _pendingImport.isbn
                         << "description chars:" << description.size();
  emit bookImportRequested(_pendingImport);
}

GlobalBookSearchController *GlobalBookSearchController::create(QQmlEngine *engine, QJSEngine *scriptEngine) {
  Q_UNUSED(engine)
  Q_UNUSED(scriptEngine)
  Q_ASSERT_X(s_instance, "GlobalBookSearchController::create",
             "setInstance() must be called before the QML engine loads");
  QQmlEngine::setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
  return s_instance;
}

void GlobalBookSearchController::setInstance(GlobalBookSearchController *instance) { s_instance = instance; }

} // namespace readary::controllers
