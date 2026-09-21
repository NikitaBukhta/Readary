#ifndef READARY_CONTROLLERS_NAVIGATIONCONTROLLER_HPP
#define READARY_CONTROLLERS_NAVIGATIONCONTROLLER_HPP

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QStack>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

#include <cstdint>

namespace readary::controllers {

class NavigationController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(QUrl currentPagePath READ currentPagePath NOTIFY currentPageChanged FINAL)
  Q_PROPERTY(Page currentPage READ currentPage WRITE setCurrentPage NOTIFY currentPageChanged FINAL)

public:
  enum class Page : std::uint8_t {
    MainPage = 1,
    CategoryListPage,
    SearchPage,
    GoalsPage,
    ChallengesPage,
    ProfilePage,
    AddBookPage,
    BookDetailPage,
    BookStatisticsPage,
  };
  Q_ENUM(Page)

  explicit NavigationController(QObject *parent);

  QUrl currentPagePath() const;
  Page currentPage() const;
  void setCurrentPage(Page page);

  Q_INVOKABLE void goBack();

  static NavigationController *create(QQmlEngine *engine, QJSEngine *scriptEngine);
  static void setInstance(NavigationController *instance);

signals:
  void currentPageChanged();

private:
  struct PageInfo {
    QUrl url;
    qint8 level;
  };

  static PageInfo pageInfo(Page page);

  static NavigationController *s_instance;

  QStack<Page> _pageStack;
};

} // namespace readary::controllers

#endif // READARY_CONTROLLERS_NAVIGATIONCONTROLLER_HPP
