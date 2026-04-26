#ifndef BEELIBRARY_CORE_APPINITIALIZER_HPP
#define BEELIBRARY_CORE_APPINITIALIZER_HPP

#include <QGuiApplication>
#include <QObject>
#include <memory>

class QQmlApplicationEngine;

namespace bl::services {
class BookTable;
}

namespace bl::models {
class BookListModel;
class BookProxyModel;
} // namespace bl::models

namespace bl::controllers {
class BookFormController;
class NavigationController;
} // namespace bl::controllers

namespace bl::core {

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
  models::BookListModel *_bookListModel;
  models::BookProxyModel *_bookProxyModel;
  controllers::BookFormController *_bookFormController;
  controllers::NavigationController *_contextModel;
};

} // namespace bl::core

#endif // BEELIBRARY_CORE_APPINITIALIZER_HPP
