#include "AppInitializer.hpp"
#include "AppEnvironment.hpp"
#include "DatabaseManager.hpp"
#include "controllers/BookFormController.hpp"
#include "controllers/NavigationController.hpp"
#include "models/BookListModel.hpp"
#include "models/BookProxyModel.hpp"

#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtQml>

Q_LOGGING_CATEGORY(lcInit, "bl.core.init")

namespace bl::core {

AppInitializer::AppInitializer(QGuiApplication &app, QObject *parent)
    : QObject(parent), _app{app},
      _engine{std::make_unique<QQmlApplicationEngine>()},
      _bookListModel{nullptr}, _bookProxyModel{nullptr},
      _bookFormController{nullptr}, _contextModel{nullptr} {}

AppInitializer::~AppInitializer() = default;

int AppInitializer::run() {
  init();
  int res = _app.exec();
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

#ifndef QT_NO_DEBUG
  _db->runScript(":/db/test_data.sql");
#endif

  _bookTable = std::make_shared<services::BookTable>(_db);

  qCInfo(lcInit) << "Database layer ready";
}

void AppInitializer::initModels() {
  _bookListModel = new models::BookListModel(_bookTable, this);

  _bookProxyModel = new models::BookProxyModel(this);
  _bookProxyModel->setSourceModel(_bookListModel);

  _bookFormController =
      new controllers::BookFormController(_bookTable, _bookListModel, this);
  connect(_bookFormController, &controllers::BookFormController::bookSaved,
          _bookListModel, &models::BookListModel::refresh);

  _contextModel = new controllers::NavigationController(this);

  qCInfo(lcInit) << "Models ready";
}

void AppInitializer::registerQmlTypes() {
  models::BookListModel::setInstance(_bookListModel);
  models::BookProxyModel::setInstance(_bookProxyModel);
  controllers::BookFormController::setInstance(_bookFormController);
  controllers::NavigationController::setInstance(_contextModel);

  QObject::connect(
      _engine.get(), &QQmlApplicationEngine::objectCreationFailed, &_app,
      []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

  _engine->loadFromModule("Library", "Main");

  qCInfo(lcInit) << "QML types registered";
}

} // namespace bl::core
