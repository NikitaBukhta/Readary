#include "BookController.hpp"
#include "models/BookListModel.hpp"
#include "models/BookSearchProxyModel.hpp"
#include "models/BookSortFilterProxyModel.hpp"
#include "models/filters/BookFilterStrategy.hpp"

#include <QDate>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcBook, "bl.controllers.book")

namespace bl::controllers {

static const QString kIsbnPrefix = QStringLiteral("ISBN-");

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
}

BookController::~BookController() = default;

void BookController::setInstance(BookController *instance) { s_instance = instance; }

BookController *BookController::create(QQmlEngine *, QJSEngine *) {
  Q_ASSERT_X(s_instance, "BookController::create", "setInstance() must be called before the QML engine loads");
  QQmlEngine::setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
  return s_instance;
}

int BookController::currentBookId() const { return _currentBookId; }

void BookController::setCurrentBookId(int id) {
  if (_currentBookId == id)
    return;
  _currentBookId = id;
  emit currentBookIdChanged();
}

QVariantMap BookController::currentBookData() const {
  if (_currentBookId <= 0)
    return {};

  auto book = _listModel->getBook(_currentBookId);
  return book;
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

bl::models::BookSearchProxyModel *BookController::searchModel() const { return _searchProxy; }

bl::models::BookSortFilterProxyModel *
BookController::buildProxy(bl::models::BookListModel *source, const bl::models::filters::BookFilterStrategy &strategy) {
  auto *proxy = new bl::models::BookSortFilterProxyModel(this);
  proxy->setSourceModel(source);
  strategy.apply(proxy);
  return proxy;
}

bl::models::BookSortFilterProxyModel *BookController::proxyFor(ListKind kind) const {
  return _proxies.value(kind, nullptr);
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
