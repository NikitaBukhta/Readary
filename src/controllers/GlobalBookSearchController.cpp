#include "GlobalBookSearchController.hpp"

#include "utils/IsbnValidator.hpp"

#include <QLoggingCategory>

namespace {
Q_LOGGING_CATEGORY(lcGlobalSearch, "readary.controllers.globalsearch")
}

namespace readary::controllers {

GlobalBookSearchController *GlobalBookSearchController::s_instance = nullptr;

GlobalBookSearchController::GlobalBookSearchController(QObject *parent)
    : QObject{parent}, _bookSearchAPI{nullptr}, _resultsModel{new models::GlobalBookSearchListModel{this}} {}

void GlobalBookSearchController::setBookSearchAPI(api::IBookSearchAPI *api) {
  _bookSearchAPI = api;
  if (_bookSearchAPI != nullptr) {
    connect(_bookSearchAPI, &api::IBookSearchAPI::searchListUpdated, _resultsModel,
            &models::GlobalBookSearchListModel::onSearchListUpdated);
  }
}

models::GlobalBookSearchListModel *GlobalBookSearchController::resultsModel() const { return _resultsModel; }

void GlobalBookSearchController::search(const QString &query) const {
  qCInfo(lcGlobalSearch) << "search query:" << query;
  if (_bookSearchAPI == nullptr) {
    qCWarning(lcGlobalSearch) << "search aborted — book search API not set";
    return;
  }

  if (const auto isbn = utils::IsbnValidator::convert(query)) {
    qCInfo(lcGlobalSearch) << "query recognized as ISBN:" << *isbn;
    _bookSearchAPI->searchByISBN(*isbn);
    return;
  }

  const api::BookSearchFields searchFields{
      .isbn = 0,
      .name = query,
      .author = query,
  };
  _bookSearchAPI->search(searchFields);
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
  qCInfo(lcGlobalSearch) << "import & open requested — isbn:" << book.isbn << "name:" << book.name;
  emit bookImportRequested(book);
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
