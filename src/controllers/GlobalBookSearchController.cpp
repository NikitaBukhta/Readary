#include "controllers/GlobalBookSearchController.hpp"

#include "api/translate/LanguageDetector.hpp"
#include "services/caching/SearchCache.hpp"
#include "utils/IsbnValidator.hpp"

#include <QLoggingCategory>

namespace {
Q_LOGGING_CATEGORY(lcGlobalSearch, "readary.controllers.globalsearch")
constexpr int g_searchCacheMaxAgeDays{7};
constexpr int g_minFilteredResults{10};
constexpr int g_maxAutoPages{5};
} // namespace

namespace readary::controllers {

GlobalBookSearchController *GlobalBookSearchController::s_instance = nullptr;

GlobalBookSearchController::GlobalBookSearchController(QObject *parent)
    : QObject{parent}, _resultsModel{new models::GlobalBookSearchListModel{this}} {}

void GlobalBookSearchController::setBookSearchAPI(api::IBookSearchAPI *api) {
  _bookSearchAPI = api;
  if (_bookSearchAPI != nullptr) {
    connect(_bookSearchAPI, &api::IBookSearchAPI::searchListUpdated, this,
            &GlobalBookSearchController::onSearchResults);
    connect(_bookSearchAPI, &api::IBookSearchAPI::descriptionReady, this,
            &GlobalBookSearchController::onDescriptionReady);
  }
}

void GlobalBookSearchController::setTranslator(api::ITranslator *translator) {
  _translator = translator;
  if (_translator != nullptr) {
    connect(_translator, &api::ITranslator::translationReady, this, &GlobalBookSearchController::onTranslationReady);
  }
}

void GlobalBookSearchController::setLanguageModel(models::LanguageModel *languageModel) {
  _languageModel = languageModel;
}

void GlobalBookSearchController::setOwnershipChecker(std::function<bool(qint64)> isOwned) {
  _isOwned = std::move(isOwned);
}

void GlobalBookSearchController::setFilterCriteria(const services::BookFilterCriteria &criteria) {
  _criteria = criteria.catalogSubset();
  if (_activeQuery.isEmpty() && !_criteria.hasCatalogTerms()) {
    qCInfo(lcGlobalSearch) << "criteria changed but nothing to query for — no search issued";
    return;
  }

  qCInfo(lcGlobalSearch) << "criteria changed — running query:" << _activeQuery;
  search(_activeQuery);
}

void GlobalBookSearchController::setPendingQuery(const QString &text) {
  _activeQuery = normalizeKey(text);
  _activeKey = cacheKey(_activeQuery);
}

QString GlobalBookSearchController::cacheKey(const QString &normalizedQuery) const {
  if (_criteria.isEmpty()) {
    return normalizedQuery;
  }
  const QStringList parts{normalizedQuery,
                          _criteria.languages.join(u'+'),
                          _criteria.genres.join(u'+'),
                          _criteria.author,
                          _criteria.publisher,
                          QString::number(_criteria.minPages),
                          QString::number(_criteria.maxPages),
                          QString::number(_criteria.minYear),
                          QString::number(_criteria.maxYear),
                          QString::number(_criteria.minRating)};
  return parts.join(u'|');
}

models::GlobalBookSearchListModel *GlobalBookSearchController::resultsModel() const { return _resultsModel; }

bool GlobalBookSearchController::isSearching() const { return _searching; }

bool GlobalBookSearchController::canLoadMore() const { return _canLoadMore; }

void GlobalBookSearchController::setSearching(bool searching) {
  if (_searching == searching) {
    return;
  }
  _searching = searching;
  emit searchingChanged();
}

void GlobalBookSearchController::setCanLoadMore(bool canLoadMore) {
  if (_canLoadMore == canLoadMore) {
    return;
  }
  _canLoadMore = canLoadMore;
  emit canLoadMoreChanged();
}

void GlobalBookSearchController::search(const QString &query) {
  const QString text = normalizeKey(query);
  if (text.isEmpty() && !_criteria.hasCatalogTerms()) {
    return;
  }
  const QString key = cacheKey(text);
  qCInfo(lcGlobalSearch) << "search query:" << text << "criteria:" << _criteria.activeCount();
  _activeQuery = text;
  _activeKey = key;
  _autoPagesFetched = 0;
  setCanLoadMore(false);

  if (tryServeFromMemoryCache(key) || tryServeFromDiskCache(key)) {
    return;
  }
  startFreshSearch(key);
}

QString GlobalBookSearchController::normalizeKey(const QString &query) { return query.trimmed().toLower(); }

bool GlobalBookSearchController::tryServeFromMemoryCache(const QString &key) {
  const auto cached = _searchCache.constFind(key);
  if (cached == _searchCache.constEnd() || cached->books.isEmpty()) {
    return false;
  }
  qCInfo(lcGlobalSearch) << "memory cache hit — returning" << cached->books.size()
                         << "cached result(s), hasMore:" << cached->hasMore;
  setSearching(false);
  _resultsModel->setBooks(cached->books);
  setCanLoadMore(cached->hasMore);
  return true;
}

bool GlobalBookSearchController::tryServeFromDiskCache(const QString &key) {
  const auto persisted = services::SearchCache::get(key, g_searchCacheMaxAgeDays);
  if (!persisted || persisted->books.isEmpty()) {
    return false;
  }
  qCInfo(lcGlobalSearch) << "disk cache hit — returning" << persisted->books.size()
                         << "cached result(s), hasMore:" << persisted->hasMore;
  _searchCache.insert(
      key, CachedSearch{.books = persisted->books, .nextPage = persisted->nextPage, .hasMore = persisted->hasMore});
  setSearching(false);
  _resultsModel->setBooks(persisted->books);
  setCanLoadMore(persisted->hasMore);
  return true;
}

void GlobalBookSearchController::startFreshSearch(const QString &key) {
  _searchCache.insert(key, CachedSearch{});
  setSearching(true);
  _resultsModel->setBooks({});
  requestPage(_activeQuery, 1);
}

void GlobalBookSearchController::loadMore() {
  if (_activeKey.isEmpty() || _searching || !_canLoadMore) {
    return;
  }
  const auto it = _searchCache.constFind(_activeKey);
  if (it == _searchCache.constEnd() || !it->hasMore) {
    return;
  }
  qCInfo(lcGlobalSearch) << "loadMore — query:" << _activeQuery << "page:" << it->nextPage;
  _autoPagesFetched = 0; // a user-driven page is not part of the auto-fetch budget
  requestPage(_activeQuery, it->nextPage);
}

void GlobalBookSearchController::requestPage(const QString &query, int page) {
  if (_bookSearchAPI == nullptr) {
    qCWarning(lcGlobalSearch) << "search aborted — book search API not set";
    setSearching(false);
    return;
  }

  setSearching(true);
  _pendingKey = cacheKey(query);
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
      .criteria = _criteria,
  };
  _bookSearchAPI->search(searchFields);
}

std::optional<qint64> GlobalBookSearchController::queryAsIsbn(const QString &query) {
  // toUpper so a lowercased ISBN-10 check digit ('x') is still recognized.
  return utils::IsbnValidator::convert(query.toUpper());
}

void GlobalBookSearchController::onSearchResults(const QList<services::BookDTO> &books, bool hasMore) {
  setSearching(false);
  if (_pendingKey.isEmpty()) {
    return;
  }

  const CachedSearch &entry = accumulatePage(_pendingKey, _pendingPage, books, hasMore);
  services::SearchCache::put(_pendingKey, {.books = entry.books, .nextPage = entry.nextPage, .hasMore = entry.hasMore});

  if (_pendingKey != _activeKey) {
    return;
  }
  showPage(books, _pendingPage <= 1);
  // hasMore only reports a full *raw* page; everything without an ISBN or a page
  // count is dropped after, so a page can arrive full and still yield no rows.
  setCanLoadMore(hasMore && !books.isEmpty());
  maybeFetchMorePages(entry, hasMore);
}

void GlobalBookSearchController::maybeFetchMorePages(const CachedSearch &entry, bool hasMore) {
  if (_criteria.isEmpty() || !hasMore || entry.books.size() >= g_minFilteredResults) {
    return;
  }
  if (_autoPagesFetched >= g_maxAutoPages) {
    qCInfo(lcGlobalSearch) << "auto-fetch budget spent after" << _autoPagesFetched << "page(s) —" << entry.books.size()
                           << "result(s) survived the filter";
    return;
  }

  ++_autoPagesFetched;
  qCInfo(lcGlobalSearch) << "only" << entry.books.size() << "result(s) after filtering — fetching page"
                         << entry.nextPage << "(auto" << _autoPagesFetched << "of" << g_maxAutoPages << ")";
  requestPage(_activeQuery, entry.nextPage);
}

GlobalBookSearchController::CachedSearch &
GlobalBookSearchController::accumulatePage(const QString &key, int page, const QList<services::BookDTO> &books,
                                           bool hasMore) {
  auto it = _searchCache.find(key);
  if (it == _searchCache.end()) {
    it = _searchCache.insert(key, CachedSearch{});
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

  if (_isOwned && _isOwned(book.isbn)) {
    qCInfo(lcGlobalSearch) << "already in library — opening without a description fetch, isbn:" << book.isbn;
    emit bookImportRequested(book);
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

  if (maybeTranslateDescription(description)) {
    return;
  }

  qCInfo(lcGlobalSearch) << "import & open requested — isbn:" << _pendingImport.isbn
                         << "description chars:" << description.size();
  emit bookImportRequested(_pendingImport);
}

bool GlobalBookSearchController::maybeTranslateDescription(const QString &description) {
  if (_translator == nullptr || _languageModel == nullptr || description.isEmpty()) {
    return false;
  }

  const QString target = models::LanguageModel::localeCode(_languageModel->current());
  const QString detected = api::detectLanguage(description);
  if (target.isEmpty() || detected.isEmpty() || detected == target) {
    return false;
  }

  _pendingTranslateId = ++_nextTranslateId;
  qCInfo(lcGlobalSearch) << "translating description — isbn:" << _pendingImport.isbn << "target:" << target
                         << "req:" << _pendingTranslateId;
  _translator->translate(description, target, _pendingTranslateId);
  return true;
}

void GlobalBookSearchController::onTranslationReady(quint64 requestId, const QString &translated) {
  if (requestId != _pendingTranslateId) {
    return;
  }
  _pendingTranslateId = 0;
  if (!translated.isEmpty()) {
    _pendingImport.description = translated;
  }

  qCInfo(lcGlobalSearch) << "import & open requested (translated) — isbn:" << _pendingImport.isbn
                         << "description chars:" << _pendingImport.description.size();
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
