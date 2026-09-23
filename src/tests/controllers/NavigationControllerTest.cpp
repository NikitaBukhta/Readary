#include "controllers/NavigationController.hpp"

#include <QSignalSpy>
#include <QTest>

using Qt::StringLiterals::operator""_s;

using readary::controllers::NavigationController;

namespace {

using Page = NavigationController::Page;

} // namespace

class NavigationControllerTest : public QObject {
  Q_OBJECT

private slots:
  void newController_startsOnTheMainPage();
  void currentPagePath_namesTheQmlFile();
  void goalsAndChallenges_shareTheMainPage();

  void setCurrentPage_toADeeperPage_pushesIt();
  void setCurrentPage_toTheSamePage_isANoOp();
  void setCurrentPage_toASiblingAtTheSameLevel_replacesIt();
  void setCurrentPage_toARootPage_unwindsTheStack();
  void setCurrentPage_downTwoLevels_keepsBothBehind();

  void addBookPage_isReplacedByTheBookItOpens();
  void settingsPage_stacksOnTopOfTheProfile();
  void readingStatisticsPage_stacksOnTopOfTheProfile();
  void readingStatisticsPage_opensABookAndComesBack();
  void statisticsPage_stacksOnTopOfTheBookItDescribes();
  void statisticsPage_leavesTheWholeStackBehindIt();

  void goBack_returnsToThePageBelow();
  void goBack_atTheRoot_isANoOp();
  void goBack_unwindsOneLevelAtATime();
};

void NavigationControllerTest::newController_startsOnTheMainPage() {
  NavigationController controller{nullptr};
  QCOMPARE(controller.currentPage(), Page::MainPage);
}

void NavigationControllerTest::currentPagePath_namesTheQmlFile() {
  NavigationController controller{nullptr};
  QCOMPARE(controller.currentPagePath(), QUrl{u"qrc:/qt/qml/Library/pages/mainPage/MainPage.qml"_s});

  controller.setCurrentPage(Page::CategoryListPage);
  QCOMPARE(controller.currentPagePath(), QUrl{u"qrc:/qt/qml/Library/pages/categoryListPage/CategoryListPage.qml"_s});

  controller.setCurrentPage(Page::BookDetailPage);
  QCOMPARE(controller.currentPagePath(), QUrl{u"qrc:/qt/qml/Library/pages/bookDetailPage/BookDetailPage.qml"_s});

  controller.setCurrentPage(Page::ProfilePage);
  QCOMPARE(controller.currentPagePath(), QUrl{u"qrc:/qt/qml/Library/pages/profilePage/ProfilePage.qml"_s});

  controller.setCurrentPage(Page::SettingsPage);
  QCOMPARE(controller.currentPagePath(), QUrl{u"qrc:/qt/qml/Library/pages/settingsPage/SettingsPage.qml"_s});

  controller.setCurrentPage(Page::SearchPage);
  QCOMPARE(controller.currentPagePath(), QUrl{u"qrc:/qt/qml/Library/pages/searchPage/SearchPage.qml"_s});

  controller.setCurrentPage(Page::AddBookPage);
  QCOMPARE(controller.currentPagePath(), QUrl{u"qrc:/qt/qml/Library/pages/addBookPage/AddBookPage.qml"_s});

  controller.setCurrentPage(Page::BookStatisticsPage);
  QCOMPARE(controller.currentPagePath(),
           QUrl{u"qrc:/qt/qml/Library/pages/bookStatisticsPage/BookStatisticsPage.qml"_s});

  controller.setCurrentPage(Page::ReadingStatisticsPage);
  QCOMPARE(controller.currentPagePath(),
           QUrl{u"qrc:/qt/qml/Library/pages/readingStatisticsPage/ReadingStatisticsPage.qml"_s});
}

void NavigationControllerTest::goalsAndChallenges_shareTheMainPage() {
  // Neither page exists yet; both land back on the main page.
  NavigationController controller{nullptr};
  controller.setCurrentPage(Page::GoalsPage);

  QCOMPARE(controller.currentPage(), Page::GoalsPage);
  QCOMPARE(controller.currentPagePath(), QUrl{u"qrc:/qt/qml/Library/pages/mainPage/MainPage.qml"_s});
}

void NavigationControllerTest::setCurrentPage_toADeeperPage_pushesIt() {
  NavigationController controller{nullptr};
  QSignalSpy pageSpy{&controller, &NavigationController::currentPageChanged};

  controller.setCurrentPage(Page::CategoryListPage);

  QCOMPARE(pageSpy.count(), 1);
  QCOMPARE(controller.currentPage(), Page::CategoryListPage);

  // The main page is still underneath.
  controller.goBack();
  QCOMPARE(controller.currentPage(), Page::MainPage);
}

void NavigationControllerTest::setCurrentPage_toTheSamePage_isANoOp() {
  NavigationController controller{nullptr};
  QSignalSpy pageSpy{&controller, &NavigationController::currentPageChanged};

  controller.setCurrentPage(Page::MainPage);

  QCOMPARE(pageSpy.count(), 0);
}

void NavigationControllerTest::setCurrentPage_toASiblingAtTheSameLevel_replacesIt() {
  // Both are root-level tabs, so one must not stack on the other.
  NavigationController controller{nullptr};
  controller.setCurrentPage(Page::SearchPage);

  QCOMPARE(controller.currentPage(), Page::SearchPage);
  controller.goBack();
  QCOMPARE(controller.currentPage(), Page::SearchPage);
}

void NavigationControllerTest::setCurrentPage_toARootPage_unwindsTheStack() {
  NavigationController controller{nullptr};
  controller.setCurrentPage(Page::CategoryListPage);
  controller.setCurrentPage(Page::BookDetailPage);

  controller.setCurrentPage(Page::ProfilePage);

  QCOMPARE(controller.currentPage(), Page::ProfilePage);
  controller.goBack();
  QCOMPARE(controller.currentPage(), Page::ProfilePage);
}

void NavigationControllerTest::setCurrentPage_downTwoLevels_keepsBothBehind() {
  NavigationController controller{nullptr};
  controller.setCurrentPage(Page::CategoryListPage);
  controller.setCurrentPage(Page::BookDetailPage);

  QCOMPARE(controller.currentPage(), Page::BookDetailPage);
  controller.goBack();
  QCOMPARE(controller.currentPage(), Page::CategoryListPage);
  controller.goBack();
  QCOMPARE(controller.currentPage(), Page::MainPage);
}

void NavigationControllerTest::addBookPage_isReplacedByTheBookItOpens() {
  // Adding a book ends by opening it, and the filled-in form must not be what
  // `goBack` lands on — the two pages share a level so the detail page replaces
  // the form.
  NavigationController controller{nullptr};
  controller.setCurrentPage(Page::SearchPage);
  controller.setCurrentPage(Page::AddBookPage);
  controller.setCurrentPage(Page::BookDetailPage);

  controller.goBack();

  QCOMPARE(controller.currentPage(), Page::SearchPage);
}

void NavigationControllerTest::settingsPage_stacksOnTopOfTheProfile() {
  NavigationController controller{nullptr};
  controller.setCurrentPage(Page::ProfilePage);

  controller.setCurrentPage(Page::SettingsPage);

  QCOMPARE(controller.currentPage(), Page::SettingsPage);
  controller.goBack();
  QCOMPARE(controller.currentPage(), Page::ProfilePage);
}

void NavigationControllerTest::readingStatisticsPage_stacksOnTopOfTheProfile() {
  NavigationController controller{nullptr};
  controller.setCurrentPage(Page::ProfilePage);

  controller.setCurrentPage(Page::ReadingStatisticsPage);

  QCOMPARE(controller.currentPage(), Page::ReadingStatisticsPage);
  controller.goBack();
  QCOMPARE(controller.currentPage(), Page::ProfilePage);
}

void NavigationControllerTest::readingStatisticsPage_opensABookAndComesBack() {
  // A row of the in-progress card opens that book; its back arrow has to land
  // on the statistics again, not on the profile.
  NavigationController controller{nullptr};
  controller.setCurrentPage(Page::ProfilePage);
  controller.setCurrentPage(Page::ReadingStatisticsPage);
  controller.setCurrentPage(Page::BookDetailPage);

  controller.goBack();

  QCOMPARE(controller.currentPage(), Page::ReadingStatisticsPage);
}

void NavigationControllerTest::statisticsPage_stacksOnTopOfTheBookItDescribes() {
  // Statistics is opened from the detail page and its back arrow has to land
  // there, so it sits one level deeper rather than replacing it.
  NavigationController controller{nullptr};
  controller.setCurrentPage(Page::BookDetailPage);

  controller.setCurrentPage(Page::BookStatisticsPage);

  QCOMPARE(controller.currentPage(), Page::BookStatisticsPage);
  controller.goBack();
  QCOMPARE(controller.currentPage(), Page::BookDetailPage);
}

void NavigationControllerTest::statisticsPage_leavesTheWholeStackBehindIt() {
  NavigationController controller{nullptr};
  controller.setCurrentPage(Page::CategoryListPage);
  controller.setCurrentPage(Page::BookDetailPage);
  controller.setCurrentPage(Page::BookStatisticsPage);

  controller.goBack();
  controller.goBack();

  QCOMPARE(controller.currentPage(), Page::CategoryListPage);
}

void NavigationControllerTest::goBack_returnsToThePageBelow() {
  NavigationController controller{nullptr};
  controller.setCurrentPage(Page::CategoryListPage);

  QSignalSpy pageSpy{&controller, &NavigationController::currentPageChanged};
  controller.goBack();

  QCOMPARE(pageSpy.count(), 1);
  QCOMPARE(controller.currentPage(), Page::MainPage);
}

void NavigationControllerTest::goBack_atTheRoot_isANoOp() {
  NavigationController controller{nullptr};

  QSignalSpy pageSpy{&controller, &NavigationController::currentPageChanged};
  controller.goBack();

  QCOMPARE(pageSpy.count(), 0);
  QCOMPARE(controller.currentPage(), Page::MainPage);
}

void NavigationControllerTest::goBack_unwindsOneLevelAtATime() {
  NavigationController controller{nullptr};
  controller.setCurrentPage(Page::CategoryListPage);
  controller.setCurrentPage(Page::BookDetailPage);

  QSignalSpy pageSpy{&controller, &NavigationController::currentPageChanged};
  controller.goBack();
  controller.goBack();
  controller.goBack();

  // The third call has nothing left to pop.
  QCOMPARE(pageSpy.count(), 2);
  QCOMPARE(controller.currentPage(), Page::MainPage);
}

QTEST_GUILESS_MAIN(NavigationControllerTest)
#include "NavigationControllerTest.moc"
