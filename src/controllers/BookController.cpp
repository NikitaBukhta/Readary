#include "BookController.hpp"
#include "models/books/BookListModel.hpp"
#include "models/books/BookSearchProxyModel.hpp"
#include "models/books/BookSortFilterProxyModel.hpp"
#include "models/books/filters/BookFilterStrategy.hpp"
#include "services/BookStatus.hpp"
#include "services/ReadingProgressCache.hpp"
#include "services/ReadingSessionCache.hpp"

#include <QLoggingCategory>

namespace {
Q_LOGGING_CATEGORY(lcBook, "readary.controllers.book")
}

namespace readary::controllers {

BookController *BookController::s_instance = nullptr;

BookController::BookController(std::shared_ptr<services::BookTable> bookTable,
                               readary::models::BookListModel *listModel, QObject *parent)
    : QObject{parent}, _bookTable{std::move(bookTable)}, _listModel{listModel},
      _searchProxy{new readary::models::BookSearchProxyModel{this}},
      _charactersModel{new readary::models::BookCharactersModel{_bookTable, this}} {
  using namespace readary::models::filters;
  _listModel->refresh();

  _proxies.insert(ListKind::WantToRead, buildProxy(_listModel, WantToReadFilterStrategy{}));
  _proxies.insert(ListKind::WantToBuy, buildProxy(_listModel, WantToBuyFilterStrategy{}));
  _proxies.insert(ListKind::AlreadyRead, buildProxy(_listModel, AlreadyReadFilterStrategy{}));
  _proxies.insert(ListKind::InProgress, buildProxy(_listModel, ReadInProgressFilterStrategy{}));

  applyActiveSourceToSearchProxy();

  QObject::connect(this, &BookController::currentBookIsbnChanged, this,
                   [this] { _charactersModel->setBookIsbn(_currentBookIsbn); });

  QObject::connect(_listModel, &QAbstractItemModel::modelReset, this, [this] {
    if (!_cacheValid)
      return;
    _cacheValid = false;
    emit currentBookIsbnChanged();
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

qint64 BookController::currentBookIsbn() const { return _currentBookIsbn; }

void BookController::setCurrentBookIsbn(qint64 isbn) {
  if (_currentBookIsbn == isbn)
    return;
  _currentBookIsbn = isbn;
  _cacheValid = false;
  emit currentBookIsbnChanged();
}

readary::qmltypes::BookDTOObject BookController::currentBookData() const {
  if (_currentBookIsbn <= 0)
    return {};

  if (_cacheValid)
    return _cachedBookData;

  auto base = _listModel->getBook(_currentBookIsbn);
  if (base.isbn == 0)
    return {};

  readary::qmltypes::BookDTOObject result(std::move(base));
  result.genres = _bookTable->getGenres(_currentBookIsbn);
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

readary::models::BookSortFilterProxyModel *BookController::getSortFilterProxyForKind(ListKind kind) const {
  return _proxies.value(kind, nullptr);
}

void BookController::openBook(qint64 isbn) {
  if (isbn <= 0)
    return;
  setCurrentBookIsbn(isbn);
  emit bookOpenRequested(isbn);
}

void BookController::importAndOpenBook(const services::BookDTO &book) {
  if (book.isbn <= 0) {
    qCWarning(lcBook) << "Cannot import book without ISBN — name:" << book.name;
    return;
  }

  if (!_listModel->contains(book.isbn)) {
    if (_bookTable->addBook(book) == 0) {
      setErrorMessage(tr("Failed to import book."));
      return;
    }
    emit bookSaved();
  }

  openBook(book.isbn);
}

void BookController::saveReadingSession(qint64 bookIsbn, int seconds, int phase) {
  if (bookIsbn <= 0)
    return;
  services::ReadingSessionCache::save(bookIsbn, seconds, phase);
}

QVariantMap BookController::takeReadingSession(qint64 bookIsbn) {
  if (bookIsbn <= 0)
    return {};
  return services::ReadingSessionCache::takeState(bookIsbn);
}

void BookController::clearReadingSession(qint64 bookIsbn) {
  if (bookIsbn <= 0)
    return;
  services::ReadingSessionCache::clear(bookIsbn);
}

void BookController::setBookStatus(int status) {
  if (_currentBookIsbn <= 0)
    return;

  qmltypes::BookDTOObject book = currentBookData();
  if (book.status == status)
    return;

  book.status = status;
  if (!_bookTable->updateBook(book)) {
    setErrorMessage(tr("Failed to update book status."));
    return;
  }

  qCInfo(lcBook) << "Set status — book isbn:" << _currentBookIsbn << "status:" << status;
  emit bookSaved();
}

void BookController::toggleWantToRead() {
  if (_currentBookIsbn <= 0)
    return;

  const int current = currentBookData().status;
  setBookStatus(current == services::BookStatus::WantToRead ? services::BookStatus::None
                                                            : services::BookStatus::WantToRead);
}

void BookController::toggleWishList() {
  if (_currentBookIsbn <= 0)
    return;

  qmltypes::BookDTOObject book = currentBookData();
  book.inWishList = !book.inWishList;
  if (!_bookTable->updateBook(book)) {
    setErrorMessage(tr("Failed to update wishlist."));
    return;
  }

  qCInfo(lcBook) << "Toggled wishlist — book isbn:" << _currentBookIsbn << "inWishList:" << book.inWishList;
  emit bookSaved();
}

void BookController::moveInProgressToWantToRead() {
  if (_currentBookIsbn <= 0)
    return;

  qmltypes::BookDTOObject book = currentBookData();
  if (book.status != services::BookStatus::InProgress)
    return;

  if (book.pagesRead > 0)
    services::ReadingProgressCache::save(_currentBookIsbn, book.pagesRead);
  services::ReadingSessionCache::clear(_currentBookIsbn);

  book.status = services::BookStatus::WantToRead;
  book.pagesRead = 0;
  if (!_bookTable->updateBook(book)) {
    setErrorMessage(tr("Failed to update book status."));
    return;
  }

  qCInfo(lcBook) << "Moved in-progress book to want-to-read — book isbn:" << _currentBookIsbn;
  emit bookSaved();
}

bool BookController::hasCachedProgress() const { return services::ReadingProgressCache::has(_currentBookIsbn); }

void BookController::restoreCachedProgress() {
  if (_currentBookIsbn <= 0 || !services::ReadingProgressCache::has(_currentBookIsbn))
    return;

  qmltypes::BookDTOObject book = currentBookData();
  book.pagesRead = services::ReadingProgressCache::takePagesRead(_currentBookIsbn);
  book.status = services::BookStatus::InProgress;
  if (!_bookTable->updateBook(book)) {
    setErrorMessage(tr("Failed to restore reading progress."));
    return;
  }

  qCInfo(lcBook) << "Restored cached progress — book isbn:" << _currentBookIsbn << "pagesRead:" << book.pagesRead;
  emit bookSaved();
}

void BookController::discardCachedProgress() const { services::ReadingProgressCache::clear(_currentBookIsbn); }

void BookController::updateReadingProgress(int pageNumber, int durationSeconds) {
  if (_currentBookIsbn <= 0)
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
      _bookTable->insertReadingSession(_currentBookIsbn, pagesFrom, pageNumber, durationSeconds) <= 0) {
    qCWarning(lcBook) << "Reading progress saved, but session log insert failed for book isbn:" << _currentBookIsbn;
  }

  emit bookSaved();
}

readary::models::BookSearchProxyModel *BookController::searchModel() const { return _searchProxy; }

readary::models::BookCharactersModel *BookController::charactersModel() const { return _charactersModel; }

readary::models::BookSortFilterProxyModel *
BookController::buildProxy(readary::models::BookListModel *source,
                           const readary::models::filters::BookFilterStrategy &strategy) {
  auto *proxy = new readary::models::BookSortFilterProxyModel{this};
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

} // namespace readary::controllers
