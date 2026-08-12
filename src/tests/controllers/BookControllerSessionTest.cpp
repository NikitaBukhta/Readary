#include "controllers/BookController.hpp"
#include "services/ReadingPhase.hpp"

#include <QSettings>
#include <QStandardPaths>
#include <QString>
#include <QTest>

using Qt::StringLiterals::operator""_L1;
using Qt::StringLiterals::operator""_s;

using readary::controllers::BookController;
using readary::services::ReadingPhase;

namespace {

// The 13-digit ISBN magnitude that was lost (arriving as 0) when passed
// through a qint64 QML invokable parameter. These methods now take the ISBN
// as a string precisely so that value survives the QML->C++ boundary; the
// string round trip is the behaviour under test.
constexpr auto kIsbn = "9780201616224"_L1;

} // namespace

class BookControllerSessionTest : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void init();

  void stringIsbn_saveThenTake_roundTrips();
  void take_missingIsbn_returnsEmpty();
  void take_consumesEntry_soSecondTakeIsEmpty();
  void clear_dropsSavedState();
  void nonNumericIsbn_isIgnored();
  void emptyIsbn_isIgnored();
};

void BookControllerSessionTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  QCoreApplication::setOrganizationName("DarieszzBooksTests");
  QCoreApplication::setOrganizationDomain("tests.darieszzbooks.local");
  QCoreApplication::setApplicationName("BookControllerSessionTest");
}

void BookControllerSessionTest::init() {
  QSettings settings;
  settings.clear();
  settings.sync();
}

void BookControllerSessionTest::stringIsbn_saveThenTake_roundTrips() {
  BookController::saveReadingSession(kIsbn, 87, ReadingPhase::Running);

  const QVariantMap out = BookController::takeReadingSession(kIsbn);
  QVERIFY2(!out.isEmpty(), "a 13-digit ISBN passed as a string must round trip");
  QCOMPARE(out.value("seconds").toInt(), 87);
  QCOMPARE(out.value("phase").toInt(), static_cast<int>(ReadingPhase::Running));
}

void BookControllerSessionTest::take_missingIsbn_returnsEmpty() {
  QVERIFY(BookController::takeReadingSession(kIsbn).isEmpty());
}

void BookControllerSessionTest::take_consumesEntry_soSecondTakeIsEmpty() {
  BookController::saveReadingSession(kIsbn, 20, ReadingPhase::Paused);
  QVERIFY(!BookController::takeReadingSession(kIsbn).isEmpty());
  QVERIFY(BookController::takeReadingSession(kIsbn).isEmpty());
}

void BookControllerSessionTest::clear_dropsSavedState() {
  BookController::saveReadingSession(kIsbn, 30, ReadingPhase::Paused);
  BookController::clearReadingSession(kIsbn);
  QVERIFY(BookController::takeReadingSession(kIsbn).isEmpty());
}

void BookControllerSessionTest::nonNumericIsbn_isIgnored() {
  BookController::saveReadingSession(u"not-a-number"_s, 10, ReadingPhase::Running);
  QVERIFY(BookController::takeReadingSession(u"not-a-number"_s).isEmpty());
}

void BookControllerSessionTest::emptyIsbn_isIgnored() {
  BookController::saveReadingSession(QString(), 10, ReadingPhase::Running);
  QVERIFY(BookController::takeReadingSession(QString()).isEmpty());
}

QTEST_GUILESS_MAIN(BookControllerSessionTest)
#include "BookControllerSessionTest.moc"
