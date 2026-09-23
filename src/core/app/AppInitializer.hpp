#ifndef READARY_CORE_APPINITIALIZER_HPP
#define READARY_CORE_APPINITIALIZER_HPP

#include <QGuiApplication>
#include <QObject>
#include <memory>

class QQmlApplicationEngine;

namespace readary::services {
class BookTable;
class BookFileStore;
} // namespace readary::services

namespace readary::models {
class BookListModel;
} // namespace readary::models

namespace readary::controllers {
class BookController;
class BookFilterController;
class BookStatisticsController;
class NavigationController;
class ProfileController;
class ReadingStatisticsController;
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
  std::shared_ptr<services::BookFileStore> _bookFileStore;

  models::BookListModel *_bookListModel{nullptr};
  controllers::BookController *_bookController{nullptr};
  controllers::NavigationController *_contextModel{nullptr};
  controllers::SettingsController *_settingsController{nullptr};
  controllers::GlobalBookSearchController *_globalSearchController{nullptr};
  controllers::BookFilterController *_filterController{nullptr};
  controllers::BookStatisticsController *_statisticsController{nullptr};
  controllers::ProfileController *_profileController{nullptr};
  controllers::ReadingStatisticsController *_readingStatisticsController{nullptr};
};

} // namespace readary::core

#endif // READARY_CORE_APPINITIALIZER_HPP
