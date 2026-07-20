#include "AppInitializer.hpp"
#include "AppEnvironment.hpp"
#include "DatabaseManager.hpp"
#include "api/bookSearch/BookSearchApiComposite.hpp"
#include "api/bookSearch/OpenLibrarySeachAPI.hpp"
#include "controllers/BookController.hpp"
#include "controllers/GlobalBookSearchController.hpp"
#include "controllers/NavigationController.hpp"
#include "controllers/SettingsController.hpp"
#include "models/books/BookListModel.hpp"
#include "models/settings/FontModel.hpp"
#include "models/settings/LanguageModel.hpp"

#include <QtQml>

namespace {
Q_LOGGING_CATEGORY(lcInit, "readary.core.init")
}

namespace readary::core {

AppInitializer::AppInitializer(QGuiApplication &app, QObject *parent)
    : QObject{parent}, _app{app}, _engine{std::make_unique<QQmlApplicationEngine>()}, _bookListModel{nullptr},
      _bookController{nullptr}, _contextModel{nullptr}, _settingsController{nullptr}, _globalSearchController{nullptr} {
}

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

  _db->runScript(":/db/init.sql");

#ifdef QT_DEBUG
  // _db->runScript(":/db/test_data.sql");
#endif

  _bookTable = std::make_shared<services::BookTable>(_db);

  qCInfo(lcInit) << "Database layer ready";
}

void AppInitializer::initModels() {
  // Internal books init;
  _bookListModel = new models::BookListModel{_bookTable, this};
  _bookController = new controllers::BookController{_bookTable, _bookListModel, this};
  connect(_bookController, &controllers::BookController::bookSaved, _bookListModel, &models::BookListModel::refresh);

  // Context init;
  _contextModel = new controllers::NavigationController{this};
  connect(_bookController, &controllers::BookController::bookOpenRequested, _contextModel, [this](qint64) {
    _contextModel->setCurrentPage(controllers::NavigationController::PageEnum::BOOK_DETAIL_PAGE);
  });

  // Global search init;
  _globalSearchController = new controllers::GlobalBookSearchController{this};
  auto *openLibraryApi = new api::OpenLibrarySeachAPI{_globalSearchController};
  QList<api::IBookSearchAPI *> searchApis{openLibraryApi};
  _globalSearchController->setBookSearchAPI(
      new api::BookSearchAPIComposite{std::move(searchApis), _globalSearchController});
  connect(_globalSearchController, &controllers::GlobalBookSearchController::bookImportRequested, _bookController,
          &controllers::BookController::importAndOpenBook);

  // Settings init;
  _settingsController = new controllers::SettingsController{this};
  _settingsController->languageModel()->applyCurrent();
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

  QObject::connect(
      _engine.get(), &QQmlApplicationEngine::objectCreationFailed, &_app, []() { QCoreApplication::exit(-1); },
      Qt::QueuedConnection);

  _engine->loadFromModule("Library", "Main");

  qCInfo(lcInit) << "QML types registered";
}

} // namespace readary::core
