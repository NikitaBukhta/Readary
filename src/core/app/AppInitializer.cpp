#include "core/app/AppInitializer.hpp"

#include "api/bookSearch/BookSearchAPIComposite.hpp"
#include "api/bookSearch/GoogleBooksSearchAPI.hpp"
#include "api/bookSearch/OpenLibrarySearchAPI.hpp"
#include "api/translate/GoogleTranslator.hpp"
#include "controllers/BookController.hpp"
#include "controllers/BookFilterController.hpp"
#include "controllers/GlobalBookSearchController.hpp"
#include "controllers/NavigationController.hpp"
#include "controllers/SettingsController.hpp"
#include "core/app/AppEnvironment.hpp"
#include "core/db/DatabaseManager.hpp"
#include "models/books/list/BookListModel.hpp"
#include "models/settings/FontModel.hpp"
#include "models/settings/LanguageModel.hpp"
#include "services/storage/BookFileStore.hpp"
#include "services/storage/BookTable.hpp"

#include <QCoreApplication>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>

using Qt::StringLiterals::operator""_s;

namespace {
Q_LOGGING_CATEGORY(lcInit, "readary.core.init")
}

namespace readary::core {

AppInitializer::AppInitializer(QGuiApplication &app, QObject *parent)
    : QObject{parent}, _app{app}, _engine{std::make_unique<QQmlApplicationEngine>()} {}

AppInitializer::~AppInitializer() = default;

int AppInitializer::run() {
  init();
  const int res = QGuiApplication::exec();
  _engine.reset();
  AppEnvironment::shutdownFileLogger();
  return res;
}

void AppInitializer::init() {
  qCInfo(lcInit) << "Initializing application...";

  initDatabase();
  initModels();
  registerQmlTypes();

  qCInfo(lcInit) << "Application initialized";
}

void AppInitializer::initDatabase() {
  _db = std::make_shared<DatabaseManager>(AppEnvironment::databasePath());
  _db->open();

  _db->runScript(u":/db/init.sql"_s);

#ifdef QT_DEBUG
  // _db->runScript(":/db/test_data.sql");
#endif

  _bookTable = std::make_shared<services::BookTable>(_db);
  _bookFileStore = std::make_shared<services::BookFileStore>(AppEnvironment::bookFilesPath());

  qCInfo(lcInit) << "Database layer ready";
}

void AppInitializer::initModels() {
  // Internal books init
  _bookListModel = new models::BookListModel{_bookTable, this};
  _bookController = new controllers::BookController{_bookTable, _bookFileStore, _bookListModel, this};
  connect(_bookController, &controllers::BookController::bookSaved, _bookListModel, &models::BookListModel::refresh);

  // Context init
  _contextModel = new controllers::NavigationController{this};
  connect(_bookController, &controllers::BookController::bookOpenRequested, _contextModel,
          [this](qint64) { _contextModel->setCurrentPage(controllers::NavigationController::Page::BookDetailPage); });

  // Global search init
  _globalSearchController = new controllers::GlobalBookSearchController{this};
  auto *openLibraryApi = new api::OpenLibrarySearchAPI{_globalSearchController};
  auto *googleBooksApi = new api::GoogleBooksSearchAPI{_globalSearchController};
  QList<api::IBookSearchAPI *> searchApis{openLibraryApi};
  auto *searchComposite = new api::BookSearchAPIComposite{std::move(searchApis), _globalSearchController};
  searchComposite->setFallbackAPI(googleBooksApi);
  _globalSearchController->setBookSearchAPI(searchComposite);
  _globalSearchController->setTranslator(new api::GoogleTranslator{_globalSearchController});
  _globalSearchController->setOwnershipChecker([this](qint64 isbn) { return _bookListModel->contains(isbn); });
  connect(_globalSearchController, &controllers::GlobalBookSearchController::bookImportRequested, _bookController,
          &controllers::BookController::importAndOpenBook);

  _filterController = new controllers::BookFilterController{this};
  _filterController->setLibraryModel(_bookListModel);
  _filterController->setSearchModel(_globalSearchController->resultsModel());
  connect(_filterController, &controllers::BookFilterController::criteriaApplied, this, [this] {
    const services::BookFilterCriteria &criteria = _filterController->criteria();
    _bookController->setFilterCriteria(criteria);
    _globalSearchController->setFilterCriteria(criteria);
  });

  // Settings init
  _settingsController = new controllers::SettingsController{this};
  _settingsController->languageModel()->applyCurrent();
  _globalSearchController->setLanguageModel(_settingsController->languageModel());
  _settingsController->fontModel()->applyCurrent();
  connect(
      _settingsController->languageModel(), &models::LanguageModel::currentChanged, this,
      [this] { _engine->retranslate(); }, Qt::QueuedConnection);

  qCInfo(lcInit) << "Models ready";
}

void AppInitializer::registerQmlTypes() {
  controllers::BookController::setInstance(_bookController);
  controllers::NavigationController::setInstance(_contextModel);
  controllers::SettingsController::setInstance(_settingsController);
  controllers::GlobalBookSearchController::setInstance(_globalSearchController);
  controllers::BookFilterController::setInstance(_filterController);

  QObject::connect(
      _engine.get(), &QQmlApplicationEngine::objectCreationFailed, &_app, []() { QCoreApplication::exit(-1); },
      Qt::QueuedConnection);

  _engine->loadFromModule("Library", "Main");

  qCInfo(lcInit) << "QML types registered";
}

} // namespace readary::core
