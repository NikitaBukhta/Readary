#include "BookController.hpp"
#include "models/books/BookListModel.hpp"
#include "models/books/BookSearchProxyModel.hpp"
#include "models/books/BookSortFilterProxyModel.hpp"
#include "models/books/filters/BookFilterStrategy.hpp"
#include "services/BookStatus.hpp"
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
      _searchProxy{new bl::models::BookSearchProxyModel(this)},
      _charactersModel{new bl::models::BookCharactersModel(_bookTable, this)} {
  using namespace bl::models::filters;

  _proxies.insert(ListKind::WantToRead, buildProxy(_listModel, WantToReadFilterStrategy{}));
  _proxies.insert(ListKind::WantToBuy, buildProxy(_listModel, WantToBuyFilterStrategy{}));
  _proxies.insert(ListKind::AlreadyRead, buildProxy(_listModel, AlreadyReadFilterStrategy{}));
  _proxies.insert(ListKind::InProgress, buildProxy(_listModel, ReadInProgressFilterStrategy{}));

  applyActiveSourceToSearchProxy();

  QObject::connect(this, &BookController::currentBookIdChanged, this,
                   [this] { _charactersModel->setBookId(_currentBookId); });

  QObject::connect(_listModel, &QAbstractItemModel::modelReset, this, [this] {
    if (!_cacheValid)
      return;
    _cacheValid = false;
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
  _cacheValid = false;
  emit currentBookIdChanged();
}

bl::qmltypes::BookDTOObject BookController::currentBookData() const {
  if (_currentBookId <= 0)
    return {};

  if (_cacheValid)
    return _cachedBookData;

  auto base = _listModel->getBook(_currentBookId);
  if (base.id == 0)
    return {};

  bl::qmltypes::BookDTOObject result(std::move(base));
  result.genres = _bookTable->getGenres(_currentBookId);
  _cachedBookData = std::move(result);
  _cacheValid = true;
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

void BookController::setBookStatus(int status) {
  if (_currentBookId <= 0)
    return;

  qmltypes::BookDTOObject book = currentBookData();
  if (book.status == status)
    return;

  book.status = status;
  if (!_bookTable->updateBook(book)) {
    setErrorMessage(tr("Failed to update book status."));
    return;
  }

  qCInfo(lcBook) << "Set status — book id:" << _currentBookId << "status:" << status;
  emit bookSaved();
}

void BookController::updateReadingProgress(int pageNumber, int durationSeconds) {
  if (_currentBookId <= 0)
    return;

  qmltypes::BookDTOObject book = currentBookData();
  const int pagesFrom = book.pagesRead;
  if (book.pagesRead >= pageNumber || pageNumber > book.totalPages) {
    qDebug(lcBook) << "Not updating reading progress — invalid page number:" << pageNumber
                   << "current pages read:" << book.pagesRead << "total pages:" << book.totalPages;
    return;
  }
  book.status = pageNumber == book.totalPages ? services::BookStatus::Finished : services::BookStatus::InProgress;
  book.pagesRead = pageNumber;

  if (!_bookTable->updateBook(book)) {
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

bl::models::BookCharactersModel *BookController::charactersModel() const { return _charactersModel; }

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
