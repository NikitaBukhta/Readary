#ifndef READARY_CORE_APPINITIALIZER_HPP
#define READARY_CORE_APPINITIALIZER_HPP

#include <QGuiApplication>
#include <QObject>
#include <memory>

class QQmlApplicationEngine;

namespace readary::services {
class BookTable;
}

namespace readary::models {
class BookListModel;
} // namespace readary::models

namespace readary::controllers {
class BookController;
class BookFilterController;
class NavigationController;
class SettingsController;
class GlobalBookSearchController;
} // namespace readary::controllers

namespace readary::core {

class DatabaseManager;

class AppInitializer : public QObject {
  Q_OBJECT

public:
  explicit AppInitializer(QGuiApplication &app, QObject *parent = nullptr);
  ~AppInitializer() override;

  int run();

private:
  void init();
  void initDatabase();
  void initModels();
  void registerQmlTypes();

  QGuiApplication &_app;
  std::unique_ptr<QQmlApplicationEngine> _engine;

  std::shared_ptr<DatabaseManager> _db;
  std::shared_ptr<services::BookTable> _bookTable;

  models::BookListModel *_bookListModel{nullptr};
  controllers::BookController *_bookController{nullptr};
  controllers::NavigationController *_contextModel{nullptr};
  controllers::SettingsController *_settingsController{nullptr};
  controllers::GlobalBookSearchController *_globalSearchController{nullptr};
  controllers::BookFilterController *_filterController{nullptr};
};

} // namespace readary::core

#endif // READARY_CORE_APPINITIALIZER_HPP
