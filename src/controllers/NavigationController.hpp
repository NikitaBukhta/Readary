#ifndef BEELIBRARY_CONTROLLERS_NAVIGATIONCONTROLLER_HPP
#define BEELIBRARY_CONTROLLERS_NAVIGATIONCONTROLLER_HPP

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QStack>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

namespace bl::controllers {

class NavigationController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(QUrl currentPagePath READ currentPagePath NOTIFY currentPageChanged FINAL)
  Q_PROPERTY(PageEnum currentPage READ currentPage WRITE setCurrentPage NOTIFY currentPageChanged FINAL)

public:
  enum class PageEnum {
    MAIN_PAGE = 1,
    CATEGORY_LIST_PAGE,
    SEARCH_PAGE,
    GOALS_PAGE,
    CHALLENGES_PAGE,
    PROFILE_PAGE,
  };
  Q_ENUM(PageEnum)

  explicit NavigationController(QObject *parent);

  QUrl currentPagePath() const;
  PageEnum currentPage() const;
  void setCurrentPage(PageEnum page);

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

  static PageInfo pageInfo(PageEnum page);

  static NavigationController *s_instance;

  QStack<PageEnum> _pageStack;
};

} // namespace bl::controllers

#endif // BEELIBRARY_CONTROLLERS_NAVIGATIONCONTROLLER_HPP
