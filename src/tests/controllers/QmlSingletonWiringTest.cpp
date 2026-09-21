#include "controllers/BookController.hpp"
#include "controllers/BookFilterController.hpp"
#include "controllers/BookStatisticsController.hpp"
#include "controllers/NavigationController.hpp"
#include "controllers/SettingsController.hpp"
#include "models/books/list/BookListModel.hpp"
#include "support/TempLibrary.hpp"

#include <QMetaMethod>
#include <QMetaProperty>
#include <QQmlEngine>
#include <QSettings>
#include <QStandardPaths>
#include <QTest>

#include <memory>

using Qt::StringLiterals::operator""_s;

using readary::controllers::BookController;
using readary::controllers::BookFilterController;
using readary::controllers::BookStatisticsController;
using readary::controllers::NavigationController;
using readary::controllers::SettingsController;
using readary::models::BookListModel;
using readary::tests::makeBook;
using readary::tests::TempLibrary;

namespace {

constexpr qint64 kIsbn = 9780201616224LL;

// QML reaches a singleton only through create(); the object it hands back has
// to be the one AppInitializer built and registered, not a fresh instance.
template <typename Controller> void verifyCreateReturnsTheRegisteredInstance(Controller *instance) {
  Controller::setInstance(instance);
  QCOMPARE(Controller::create(nullptr, nullptr), instance);
  // Twice: QML may resolve the singleton per engine, and every resolution has
  // to land on the same object or controller state would silently fork.
  QCOMPARE(Controller::create(nullptr, nullptr), instance);
}

// The engine must never delete a controller AppInitializer owns and parents.
//
// Ownership is forced to JavaScriptOwnership first on purpose: CppOwnership is
// already the default for an object C++ created, so asserting it straight after
// create() would hold even if create() never set it. Flipping it first is what
// makes this a test of create() rather than of Qt's default.
template <typename Controller> void verifyCppKeepsOwnership(Controller *instance) {
  QQmlEngine::setObjectOwnership(instance, QQmlEngine::JavaScriptOwnership);
  QCOMPARE(QQmlEngine::objectOwnership(instance), QQmlEngine::JavaScriptOwnership);

  Controller::setInstance(instance);
  Controller::create(nullptr, nullptr);

  QCOMPARE(QQmlEngine::objectOwnership(instance), QQmlEngine::CppOwnership);
}

bool hasQmlProperty(const QMetaObject &meta, const char *name) { return meta.indexOfProperty(name) >= 0; }

// Looked up by name rather than by signature: the metaobject stores normalised
// type spellings ("qlonglong" for qint64), which a literal signature string has
// to guess at.
QMetaMethod findMethod(const QMetaObject &meta, const char *name) {
  for (int i = 0; i < meta.methodCount(); ++i) {
    const QMetaMethod method = meta.method(i);
    if (method.name() == name) {
      return method;
    }
  }
  return {};
}

} // namespace

// The singleton wiring is a convention rather than a compiler-checked contract:
// AppInitializer constructs the instance, calls setInstance(), and QML's
// create() factory hands that same instance back with C++ ownership. These
// tests pin the half that a mistake would otherwise only show at runtime.
class QmlSingletonWiringTest : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void init();
  void cleanup();

  void bookController_createReturnsTheRegisteredInstance();
  void bookController_staysOwnedByCpp();
  void navigationController_createReturnsTheRegisteredInstance();
  void navigationController_staysOwnedByCpp();
  void settingsController_createReturnsTheRegisteredInstance();
  void settingsController_staysOwnedByCpp();
  void bookFilterController_createReturnsTheRegisteredInstance();
  void bookFilterController_staysOwnedByCpp();
  void bookStatisticsController_createReturnsTheRegisteredInstance();
  void bookStatisticsController_staysOwnedByCpp();

  void controllers_exposeTheirModelsToQml();
  void bookController_exposesItsQmlInvokables();
  void navigationController_exposesItsPageApi();

private:
  TempLibrary _library;
  std::unique_ptr<BookListModel> _listModel;
  std::unique_ptr<BookController> _bookController;
};

void QmlSingletonWiringTest::initTestCase() {
  // Resources of a static lib can be stripped by the linker — see main.cpp.
  Q_INIT_RESOURCE(db_scripts);

  QStandardPaths::setTestModeEnabled(true);
  QCoreApplication::setOrganizationName("DarieszzBooksTests");
  QCoreApplication::setOrganizationDomain("tests.darieszzbooks.local");
  QCoreApplication::setApplicationName("QmlSingletonWiringTest");
}

void QmlSingletonWiringTest::init() {
  QSettings settings;
  settings.clear();
  settings.sync();

  QVERIFY(_library.open());
  QCOMPARE(_library.books()->addBook(makeBook(kIsbn, u"Refactoring"_s)), kIsbn);

  _listModel = std::make_unique<BookListModel>(_library.books(), nullptr);
  _bookController = std::make_unique<BookController>(_library.books(), _library.files(), _listModel.get(), nullptr);
}

void QmlSingletonWiringTest::cleanup() {
  // The registered pointer must not outlive the object it names.
  BookController::setInstance(nullptr);
  _bookController.reset();
  _listModel.reset();
  _library.close();
}

void QmlSingletonWiringTest::bookController_createReturnsTheRegisteredInstance() {
  verifyCreateReturnsTheRegisteredInstance(_bookController.get());
}

void QmlSingletonWiringTest::bookController_staysOwnedByCpp() { verifyCppKeepsOwnership(_bookController.get()); }

void QmlSingletonWiringTest::navigationController_createReturnsTheRegisteredInstance() {
  NavigationController controller{nullptr};
  verifyCreateReturnsTheRegisteredInstance(&controller);
  NavigationController::setInstance(nullptr);
}

void QmlSingletonWiringTest::navigationController_staysOwnedByCpp() {
  NavigationController controller{nullptr};
  verifyCppKeepsOwnership(&controller);
  NavigationController::setInstance(nullptr);
}

void QmlSingletonWiringTest::settingsController_createReturnsTheRegisteredInstance() {
  SettingsController controller;
  verifyCreateReturnsTheRegisteredInstance(&controller);
  SettingsController::setInstance(nullptr);
}

void QmlSingletonWiringTest::settingsController_staysOwnedByCpp() {
  SettingsController controller;
  verifyCppKeepsOwnership(&controller);
  SettingsController::setInstance(nullptr);
}

void QmlSingletonWiringTest::bookFilterController_createReturnsTheRegisteredInstance() {
  BookFilterController controller{nullptr};
  verifyCreateReturnsTheRegisteredInstance(&controller);
  BookFilterController::setInstance(nullptr);
}

void QmlSingletonWiringTest::bookFilterController_staysOwnedByCpp() {
  BookFilterController controller{nullptr};
  verifyCppKeepsOwnership(&controller);
  BookFilterController::setInstance(nullptr);
}

void QmlSingletonWiringTest::bookStatisticsController_createReturnsTheRegisteredInstance() {
  BookStatisticsController controller{_library.books(), nullptr};
  verifyCreateReturnsTheRegisteredInstance(&controller);
  BookStatisticsController::setInstance(nullptr);
}

void QmlSingletonWiringTest::bookStatisticsController_staysOwnedByCpp() {
  BookStatisticsController controller{_library.books(), nullptr};
  verifyCppKeepsOwnership(&controller);
  BookStatisticsController::setInstance(nullptr);
}

void QmlSingletonWiringTest::controllers_exposeTheirModelsToQml() {
  // QML never touches a model directly — it reads one off a controller
  // property. Renaming a property silently breaks every binding on it.
  const QMetaObject &book = BookController::staticMetaObject;
  QVERIFY(hasQmlProperty(book, "searchModel"));
  QVERIFY(hasQmlProperty(book, "charactersModel"));
  QVERIFY(hasQmlProperty(book, "readingHistoryModel"));
  QVERIFY(hasQmlProperty(book, "currentBookIsbn"));
  QVERIFY(hasQmlProperty(book, "currentBookData"));
  QVERIFY(hasQmlProperty(book, "activeKind"));

  const QMetaObject &settings = SettingsController::staticMetaObject;
  QVERIFY(hasQmlProperty(settings, "languageModel"));
  QVERIFY(hasQmlProperty(settings, "fontModel"));

  const QMetaObject &statistics = BookStatisticsController::staticMetaObject;
  QVERIFY(hasQmlProperty(statistics, "statistics"));
  // The ISBN is pushed in by AppInitializer and never read from QML, so it
  // deliberately is not a property.
  QVERIFY(!hasQmlProperty(statistics, "bookIsbn"));
  QVERIFY(hasQmlProperty(statistics, "hasData"));
  QVERIFY(findMethod(statistics, "refresh").isValid());

  const QMetaObject &filter = BookFilterController::staticMetaObject;
  QVERIFY(hasQmlProperty(filter, "availableGenres"));
  QVERIFY(hasQmlProperty(filter, "selectedGenres"));
  QVERIFY(hasQmlProperty(filter, "draftCount"));
  QVERIFY(hasQmlProperty(filter, "activeCount"));
}

void QmlSingletonWiringTest::bookController_exposesItsQmlInvokables() {
  const QMetaObject &meta = BookController::staticMetaObject;

  for (const char *name :
       {"openBook", "setBookStatus", "toggleWantToRead", "toggleWishList", "moveInProgressToWantToRead",
        "hasCachedProgress", "restoreCachedProgress", "discardCachedProgress", "updateReadingProgress",
        "deleteReadingSession", "saveReadingSession", "takeReadingSession", "clearReadingSession"}) {
    QVERIFY2(findMethod(meta, name).isValid(), name);
  }

  // The ISBN crosses as a string on purpose: a qint64 QML signal/invokable
  // parameter loses the magnitude of a 13-digit ISBN. Pin the parameter types
  // of the methods that decision applies to.
  for (const char *name : {"saveReadingSession", "takeReadingSession", "clearReadingSession"}) {
    const QMetaMethod method = findMethod(meta, name);
    QVERIFY2(method.parameterCount() >= 1, name);
    QCOMPARE(method.parameterType(0), static_cast<int>(QMetaType::QString));
  }
  QCOMPARE(findMethod(meta, "deleteReadingSession").parameterType(0), static_cast<int>(QMetaType::QString));

  QCOMPARE(findMethod(meta, "updateReadingProgress").parameterCount(), 2);
  QCOMPARE(findMethod(meta, "setBookStatus").parameterType(0), static_cast<int>(QMetaType::Int));
}

void QmlSingletonWiringTest::navigationController_exposesItsPageApi() {
  const QMetaObject &meta = NavigationController::staticMetaObject;
  QVERIFY(hasQmlProperty(meta, "currentPagePath"));
  QVERIFY(hasQmlProperty(meta, "currentPage"));
  QVERIFY(findMethod(meta, "goBack").isValid());
}

QTEST_GUILESS_MAIN(QmlSingletonWiringTest)
#include "QmlSingletonWiringTest.moc"
