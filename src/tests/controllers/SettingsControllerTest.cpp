#include "controllers/SettingsController.hpp"
#include "models/settings/FontModel.hpp"
#include "models/settings/LanguageModel.hpp"

#include <QMetaProperty>
#include <QSettings>
#include <QStandardPaths>
#include <QTest>

using readary::controllers::SettingsController;

class SettingsControllerTest : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void init();

  void controller_ownsBothSettingsModels();
  void models_areParentedToTheController();
  void models_areStableAcrossReads();
  void languageModel_startsOnTheStoredLanguage();
  void fontModel_startsOnTheStoredFont();
};

void SettingsControllerTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  QCoreApplication::setOrganizationName("DarieszzBooksTests");
  QCoreApplication::setOrganizationDomain("tests.darieszzbooks.local");
  QCoreApplication::setApplicationName("SettingsControllerTest");
}

void SettingsControllerTest::init() {
  QSettings settings;
  settings.clear();
  settings.sync();
}

void SettingsControllerTest::controller_ownsBothSettingsModels() {
  SettingsController controller;

  QVERIFY(controller.languageModel() != nullptr);
  QVERIFY(controller.fontModel() != nullptr);
}

void SettingsControllerTest::models_areParentedToTheController() {
  // QML only ever sees them through the controller, so their lifetime is its own.
  SettingsController controller;

  QCOMPARE(controller.languageModel()->parent(), &controller);
  QCOMPARE(controller.fontModel()->parent(), &controller);
}

void SettingsControllerTest::models_areStableAcrossReads() {
  // Both properties are CONSTANT, so QML binds them once and never re-reads.
  // Go through the metaobject, which is the path QML actually takes — a getter
  // comparison would pass even if the Q_PROPERTY named the wrong function.
  SettingsController controller;

  const QMetaObject &meta = SettingsController::staticMetaObject;
  const QMetaProperty language = meta.property(meta.indexOfProperty("languageModel"));
  const QMetaProperty font = meta.property(meta.indexOfProperty("fontModel"));
  QVERIFY(language.isValid());
  QVERIFY(font.isValid());
  QVERIFY(language.isConstant());
  QVERIFY(font.isConstant());

  QCOMPARE(language.read(&controller).value<readary::models::LanguageModel *>(), controller.languageModel());
  QCOMPARE(font.read(&controller).value<readary::models::FontModel *>(), controller.fontModel());
}

void SettingsControllerTest::languageModel_startsOnTheStoredLanguage() {
  {
    QSettings settings;
    settings.setValue("ui/language", static_cast<int>(readary::models::LanguageModel::Code::Ukrainian));
    settings.sync();
  }

  SettingsController controller;

  QCOMPARE(controller.languageModel()->current(), readary::models::LanguageModel::Code::Ukrainian);
}

void SettingsControllerTest::fontModel_startsOnTheStoredFont() {
  {
    QSettings settings;
    settings.setValue("ui/font", static_cast<int>(readary::models::FontModel::Code::NotoColorEmoji));
    settings.sync();
  }

  SettingsController controller;

  QCOMPARE(controller.fontModel()->current(), readary::models::FontModel::Code::NotoColorEmoji);
}

QTEST_GUILESS_MAIN(SettingsControllerTest)
#include "SettingsControllerTest.moc"
