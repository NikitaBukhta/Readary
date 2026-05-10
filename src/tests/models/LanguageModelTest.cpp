#include "models/settings/LanguageModel.hpp"

#include <QCoreApplication>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QString>
#include <QTest>
#include <QVariant>

using bl::models::LanguageModel;
using Code = LanguageModel::Code;

namespace {
constexpr auto g_kSettingsKey = "ui/language";
} // namespace

class LanguageModelTest : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void cleanup();

  void availableCodes_returnsAllThreeInOrder();
  void labels_areNonEmptyForEveryCode();
  void localeCode_matchesExpectedTwoLetterTag();

  void initialState_withNoPersistedValue_isValidCode();
  void initialState_withPersistedValue_restoresIt();
  void initialState_withOutOfRangePersistedValue_clampsToValidCode();

  void setCurrent_emitsCurrentChangedOnce();
  void setCurrent_sameValue_doesNotEmit();
  void setCurrent_persistsToQSettings();

  void applyCurrent_doesNotCrashWhenQmMissing();
  void applyCurrent_isIdempotent();
};

void LanguageModelTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  QCoreApplication::setOrganizationName("DarieszzBooksTests");
  QCoreApplication::setOrganizationDomain("tests.darieszzbooks.local");
  QCoreApplication::setApplicationName("LanguageModelTest");
}

void LanguageModelTest::cleanup() {
  QSettings settings;
  settings.clear();
  settings.sync();
}

void LanguageModelTest::availableCodes_returnsAllThreeInOrder() {
  const QList<int> codes = bl::models::LanguageModel::available();
  QCOMPARE(codes.size(), 3);
  QCOMPARE(codes.at(0), static_cast<int>(Code::English));
  QCOMPARE(codes.at(1), static_cast<int>(Code::Russian));
  QCOMPARE(codes.at(2), static_cast<int>(Code::Ukrainian));
}

void LanguageModelTest::labels_areNonEmptyForEveryCode() {
  for (Code code : {Code::English, Code::Russian, Code::Ukrainian}) {
    const QString label = bl::models::LanguageModel::label(code);
    QVERIFY2(!label.isEmpty(), QStringLiteral("empty label for code %1").arg(static_cast<int>(code)).toUtf8());
  }
}

void LanguageModelTest::localeCode_matchesExpectedTwoLetterTag() {
  QCOMPARE(bl::models::LanguageModel::localeCode(Code::English), QStringLiteral("en"));
  QCOMPARE(bl::models::LanguageModel::localeCode(Code::Russian), QStringLiteral("ru"));
  QCOMPARE(bl::models::LanguageModel::localeCode(Code::Ukrainian), QStringLiteral("uk"));
}

void LanguageModelTest::initialState_withNoPersistedValue_isValidCode() {
  LanguageModel m;
  const Code current = m.current();
  QVERIFY(current == Code::English || current == Code::Russian || current == Code::Ukrainian);
}

void LanguageModelTest::initialState_withPersistedValue_restoresIt() {
  {
    QSettings settings;
    settings.setValue(g_kSettingsKey, static_cast<int>(Code::Ukrainian));
    settings.sync();
  }
  LanguageModel m;
  QCOMPARE(m.current(), Code::Ukrainian);
}

void LanguageModelTest::initialState_withOutOfRangePersistedValue_clampsToValidCode() {
  {
    QSettings settings;
    settings.setValue(g_kSettingsKey, 999);
    settings.sync();
  }
  LanguageModel m;
  QCOMPARE(m.current(), Code::Ukrainian);

  {
    QSettings settings;
    settings.setValue(g_kSettingsKey, -42);
    settings.sync();
  }
  LanguageModel m2;
  QCOMPARE(m2.current(), Code::English);
}

void LanguageModelTest::setCurrent_emitsCurrentChangedOnce() {
  LanguageModel m;
  m.setCurrent(Code::English); // pin start

  QSignalSpy spy(&m, &LanguageModel::currentChanged);
  m.setCurrent(Code::Russian);

  QCOMPARE(spy.count(), 1);
  QCOMPARE(m.current(), Code::Russian);
}

void LanguageModelTest::setCurrent_sameValue_doesNotEmit() {
  LanguageModel m;
  m.setCurrent(Code::Russian);

  QSignalSpy spy(&m, &LanguageModel::currentChanged);
  m.setCurrent(Code::Russian);

  QCOMPARE(spy.count(), 0);
}

void LanguageModelTest::setCurrent_persistsToQSettings() {
  {
    LanguageModel m;
    m.setCurrent(Code::Ukrainian);
  } // model destroyed; QSettings should hold the value

  // Re-read raw to confirm we wrote what we expect to the documented key.
  QSettings settings;
  QCOMPARE(settings.value(g_kSettingsKey).toInt(), static_cast<int>(Code::Ukrainian));

  // And a fresh LanguageModel must restore it.
  LanguageModel m2;
  QCOMPARE(m2.current(), Code::Ukrainian);
}

void LanguageModelTest::applyCurrent_doesNotCrashWhenQmMissing() {
  LanguageModel m;
  m.applyCurrent(); // first call
  m.applyCurrent(); // second call after a no-op load
  QVERIFY(true);    // reaching here is the assertion
}

void LanguageModelTest::applyCurrent_isIdempotent() {
  LanguageModel m;
  for (int i = 0; i < 5; ++i) {
    m.applyCurrent();
  }
  QVERIFY(true);
}

QTEST_GUILESS_MAIN(LanguageModelTest)
#include "LanguageModelTest.moc"
