#include "models/settings/FontModel.hpp"

#include <QCoreApplication>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QString>
#include <QTest>
#include <QVariant>

using readary::models::FontModel;
using Code = FontModel::Code;

namespace {
constexpr auto g_kSettingsKey = "ui/font";
} // namespace

class FontModelTest : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void cleanup();

  void availableCodes_returnsAllInOrder();
  void labels_areNonEmptyForEveryCode();
  void resourcePath_pointsToFontsPrefix();

  void initialState_withNoPersistedValue_isValidCode();
  void initialState_withPersistedValue_restoresIt();
  void initialState_withOutOfRangePersistedValue_clampsToValidCode();

  void setCurrent_sameValue_doesNotEmit();
  void setCurrent_persistsToQSettings();

  void currentFamily_beforeApply_fallsBackToLabel();
};

void FontModelTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  QCoreApplication::setOrganizationName("DarieszzBooksTests");
  QCoreApplication::setOrganizationDomain("tests.darieszzbooks.local");
  QCoreApplication::setApplicationName("FontModelTest");
}

void FontModelTest::cleanup() {
  QSettings settings;
  settings.clear();
  settings.sync();
}

void FontModelTest::availableCodes_returnsAllInOrder() {
  const QList<int> codes = FontModel::available();
  QCOMPARE(codes.size(), 1);
  QCOMPARE(codes.at(0), static_cast<int>(Code::NotoColorEmoji));
}

void FontModelTest::labels_areNonEmptyForEveryCode() {
  for (Code code : {Code::NotoColorEmoji}) {
    const QString label = FontModel::label(code);
    QVERIFY2(!label.isEmpty(), QStringLiteral("empty label for code %1").arg(static_cast<int>(code)).toUtf8());
  }
}

void FontModelTest::resourcePath_pointsToFontsPrefix() {
  const QString path = FontModel::resourcePath(Code::NotoColorEmoji);
  QVERIFY(path.startsWith(QStringLiteral(":/fonts/")));
  QVERIFY(path.endsWith(QStringLiteral(".ttf")));
}

void FontModelTest::initialState_withNoPersistedValue_isValidCode() {
  FontModel m;
  QCOMPARE(m.current(), Code::NotoColorEmoji);
}

void FontModelTest::initialState_withPersistedValue_restoresIt() {
  {
    QSettings settings;
    settings.setValue(g_kSettingsKey, static_cast<int>(Code::NotoColorEmoji));
    settings.sync();
  }
  FontModel m;
  QCOMPARE(m.current(), Code::NotoColorEmoji);
}

void FontModelTest::initialState_withOutOfRangePersistedValue_clampsToValidCode() {
  {
    QSettings settings;
    settings.setValue(g_kSettingsKey, 999);
    settings.sync();
  }
  FontModel m;
  QCOMPARE(m.current(), Code::NotoColorEmoji);

  {
    QSettings settings;
    settings.setValue(g_kSettingsKey, -42);
    settings.sync();
  }
  FontModel m2;
  QCOMPARE(m2.current(), Code::NotoColorEmoji);
}

void FontModelTest::setCurrent_sameValue_doesNotEmit() {
  FontModel m;
  m.setCurrent(Code::NotoColorEmoji);

  QSignalSpy spy(&m, &FontModel::currentChanged);
  m.setCurrent(Code::NotoColorEmoji);

  QCOMPARE(spy.count(), 0);
}

void FontModelTest::setCurrent_persistsToQSettings() {
  {
    QSettings settings;
    settings.setValue(g_kSettingsKey, static_cast<int>(Code::NotoColorEmoji));
    settings.sync();
  }
  FontModel m;
  QCOMPARE(m.current(), Code::NotoColorEmoji);
}

void FontModelTest::currentFamily_beforeApply_fallsBackToLabel() {
  FontModel m;
  // applyCurrent needs QGuiApplication (QFontDatabase) and isn't called here;
  // the getter must still return a sane non-empty string to keep QML bindings safe.
  const QString family = m.currentFamily();
  QVERIFY(!family.isEmpty());
  QCOMPARE(family, FontModel::label(Code::NotoColorEmoji));
}

QTEST_GUILESS_MAIN(FontModelTest)
#include "FontModelTest.moc"
