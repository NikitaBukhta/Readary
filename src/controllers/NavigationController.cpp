#include "NavigationController.hpp"

#include <QLoggingCategory>
#include <QQmlEngine>

using namespace Qt::StringLiterals;

namespace {
Q_LOGGING_CATEGORY(lcNavigation, "bl.controllers.navigation")
}

namespace bl::controllers {

NavigationController *NavigationController::s_instance = nullptr;

NavigationController::NavigationController(QObject *parent) : QObject(parent) {
  setCurrentPage(PageEnum::MAIN_PAGE);
  qCInfo(lcNavigation) << "NavigationController initialized";
}

void NavigationController::setInstance(NavigationController *instance) { s_instance = instance; }

NavigationController *NavigationController::create(QQmlEngine *engine, QJSEngine *scriptEngine) {
  Q_UNUSED(engine)
  Q_UNUSED(scriptEngine)
  Q_ASSERT_X(s_instance, "NavigationController::create", "setInstance() must be called before the QML engine loads");
  QQmlEngine::setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
  return s_instance;
}

NavigationController::PageInfo NavigationController::pageInfo(PageEnum page) {
  switch (page) {
  case PageEnum::MAIN_PAGE:
    return {QUrl{u"qrc:/qt/qml/Library/pages/mainPage/MainPage.qml"_s}, 1};
  case PageEnum::CATEGORY_LIST_PAGE:
    return {QUrl{u"qrc:/qt/qml/Library/pages/categoryListPage/CategoryListPage.qml"_s}, 2};
  case PageEnum::BOOK_DETAIL_PAGE:
    return {QUrl{u"qrc:/qt/qml/Library/pages/bookDetailPage/BookDetailPage.qml"_s}, 3};
  case PageEnum::PROFILE_PAGE:
    return {QUrl{u"qrc:/qt/qml/Library/pages/settingsPage/SettingsPage.qml"_s}, 1};
  case PageEnum::SEARCH_PAGE:
  case PageEnum::GOALS_PAGE:
  case PageEnum::CHALLENGES_PAGE:
    return {QUrl{u"qrc:/qt/qml/Library/pages/mainPage/MainPage.qml"_s}, 1};
  }
  Q_UNREACHABLE_RETURN({});
}

QUrl NavigationController::currentPagePath() const {
  if (_pageStack.empty())
    return {};
  return pageInfo(_pageStack.top()).url;
}

NavigationController::PageEnum NavigationController::currentPage() const {
  if (_pageStack.empty())
    return PageEnum::MAIN_PAGE;
  return _pageStack.top();
}

void NavigationController::setCurrentPage(PageEnum page) {
  if (!_pageStack.empty() && _pageStack.top() == page)
    return;

  const auto target = pageInfo(page);

  while (!_pageStack.empty() && target.level <= pageInfo(_pageStack.top()).level) {
    _pageStack.pop();
  }

  _pageStack.push(page);
  qCInfo(lcNavigation) << "Page changed:" << target.url << "(stack size:" << _pageStack.size() << ")";
  emit currentPageChanged();
}

void NavigationController::goBack() {
  if (_pageStack.size() <= 1)
    return;

  _pageStack.pop();
  qCInfo(lcNavigation) << "Navigated back to:" << pageInfo(_pageStack.top()).url << "(stack size:" << _pageStack.size()
                       << ")";
  emit currentPageChanged();
}

} // namespace bl::controllers
