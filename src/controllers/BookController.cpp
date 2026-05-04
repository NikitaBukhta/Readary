#include "BookController.hpp"
#include "models/BookListModel.hpp"
#include "models/BookSearchProxyModel.hpp"
#include "models/BookSortFilterProxyModel.hpp"
#include "models/filters/BookFilterStrategy.hpp"
#include "services/ReadingSessionCache.hpp"

#include <QLoggingCategory>

namespace {
Q_LOGGING_CATEGORY(lcBook, "bl.controllers.book")
}

namespace bl::controllers {

BookController *BookController::s_instance = nullptr;

BookController::BookController(std::shared_ptr<services::BookTable> bookTable, bl::models::BookListModel *listModel,
                               QObject *parent)
    : QObject(parent), _bookTable{std::move(bookTable)}, _listModel{listModel},
      _searchProxy{new bl::models::BookSearchProxyModel(this)} {
  using namespace bl::models::filters;

  _proxies.insert(ListKind::WantToRead, buildProxy(_listModel, WantToReadFilterStrategy{}));
  _proxies.insert(ListKind::WantToBuy, buildProxy(_listModel, WantToBuyFilterStrategy{}));
  _proxies.insert(ListKind::AlreadyRead, buildProxy(_listModel, AlreadyReadFilterStrategy{}));
  _proxies.insert(ListKind::InProgress, buildProxy(_listModel, ReadInProgressFilterStrategy{}));

  applyActiveSourceToSearchProxy();

  QObject::connect(_listModel, &QAbstractItemModel::modelReset, this, [this] {
    if (_cachedBookData.isEmpty())
      return;
    _cachedBookData.clear();
    emit currentBookIdChanged();
  });
}

BookController::~BookController() = default;

void BookController::setInstance(BookController *instance) { s_instance = instance; }

BookController *BookController::create(QQmlEngine *engine, QJSEngine *scriptEngine) {
  Q_UNUSED(engine)
  Q_UNUSED(scriptEngine)
  Q_ASSERT_X(s_instance, "BookController::create", "setInstance() must be called before the QML engine loads");
  QQmlEngine::setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
  return s_instance;
}

qint64 BookController::currentBookId() const { return _currentBookId; }

void BookController::setCurrentBookId(qint64 id) {
  if (_currentBookId == id)
    return;
  _currentBookId = id;
  _cachedBookData.clear();
  emit currentBookIdChanged();
}

QVariantMap BookController::currentBookData() const {
  if (_currentBookId <= 0)
    return {};

  if (!_cachedBookData.isEmpty())
    return _cachedBookData;

  auto book = _listModel->getBook(_currentBookId);
  if (book.isEmpty())
    return book;

  book.insert("genres", _bookTable->getGenres(_currentBookId));
  book.insert("characters", _bookTable->getCharacters(_currentBookId));
  _cachedBookData = book;
  return _cachedBookData;
}

QString BookController::errorMessage() const { return _errorMessage; }

BookController::ListKind BookController::activeKind() const { return _activeKind; }

void BookController::setActiveKind(ListKind kind) {
  if (_activeKind == kind)
    return;
  _activeKind = kind;
  applyActiveSourceToSearchProxy();
  emit activeKindChanged();
}

bl::models::BookSortFilterProxyModel *BookController::getSortFilterProxyForKind(ListKind kind) const {
  return _proxies.value(kind, nullptr);
}

void BookController::openBook(qint64 id) {
  if (id <= 0)
    return;
  setCurrentBookId(id);
  emit bookOpenRequested(id);
}

void BookController::saveReadingSession(qint64 bookId, int seconds, int phase) {
  if (bookId <= 0)
    return;
  services::ReadingSessionCache::save(bookId, seconds, phase);
}

QVariantMap BookController::takeReadingSession(qint64 bookId) {
  if (bookId <= 0)
    return {};
  return services::ReadingSessionCache::takeState(bookId);
}

void BookController::clearReadingSession(qint64 bookId) {
  if (bookId <= 0)
    return;
  services::ReadingSessionCache::clear(bookId);
}

void BookController::updateReadingProgress(int pageNumber, int durationSeconds) {
  if (_currentBookId <= 0)
    return;

  const auto book = _listModel->getBook(_currentBookId);
  const int pagesFrom = book.value("pagesRead").toInt();

  if (!_bookTable->updatePagesRead(_currentBookId, pageNumber)) {
    setErrorMessage(tr("Failed to update reading progress."));
    return;
  }

  if (durationSeconds > 0 &&
      _bookTable->insertReadingSession(_currentBookId, pagesFrom, pageNumber, durationSeconds) <= 0) {
    qCWarning(lcBook) << "Reading progress saved, but session log insert failed for book id:" << _currentBookId;
  }

  emit bookSaved();
}

bl::models::BookSearchProxyModel *BookController::searchModel() const { return _searchProxy; }

bl::models::BookSortFilterProxyModel *
BookController::buildProxy(bl::models::BookListModel *source, const bl::models::filters::BookFilterStrategy &strategy) {
  auto *proxy = new bl::models::BookSortFilterProxyModel(this);
  proxy->setSourceModel(source);
  strategy.apply(proxy);
  return proxy;
}

void BookController::applyActiveSourceToSearchProxy() {
  if (!_searchProxy)
    return;
  _searchProxy->setSourceModel(getSortFilterProxyForKind(_activeKind));
}

void BookController::setErrorMessage(const QString &message) {
  if (_errorMessage == message)
    return;
  _errorMessage = message;
  emit errorMessageChanged();
}

} // namespace bl::controllers
