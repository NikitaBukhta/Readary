#include "SettingsController.hpp"

#include <QLoggingCategory>

namespace {
Q_LOGGING_CATEGORY(lcSettings, "readary.controllers.settings")
}

namespace readary::controllers {

SettingsController *SettingsController::s_instance = nullptr;

SettingsController::SettingsController(QObject *parent)
    : QObject{parent}, _languageModel{new readary::models::LanguageModel{this}},
      _fontModel{new readary::models::FontModel{this}} {
  qCInfo(lcSettings) << "SettingsController initialized";
}

SettingsController::~SettingsController() = default;

readary::models::LanguageModel *SettingsController::languageModel() const { return _languageModel; }

readary::models::FontModel *SettingsController::fontModel() const { return _fontModel; }

void SettingsController::setInstance(SettingsController *instance) { s_instance = instance; }

SettingsController *SettingsController::create(QQmlEngine *engine, QJSEngine *scriptEngine) {
  Q_UNUSED(engine)
  Q_UNUSED(scriptEngine)
  Q_ASSERT_X(s_instance, "SettingsController::create", "setInstance() must be called before the QML engine loads");
  QQmlEngine::setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
  return s_instance;
}

} // namespace readary::controllers
