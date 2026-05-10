#include "SettingsController.hpp"

#include <QLoggingCategory>

namespace {
Q_LOGGING_CATEGORY(lcSettings, "bl.controllers.settings")
}

namespace bl::controllers {

SettingsController *SettingsController::s_instance = nullptr;

SettingsController::SettingsController(QObject *parent)
    : QObject(parent), _languageModel{new bl::models::LanguageModel(this)} {
  qCInfo(lcSettings) << "SettingsController initialized";
}

SettingsController::~SettingsController() = default;

bl::models::LanguageModel *SettingsController::languageModel() const { return _languageModel; }

void SettingsController::setInstance(SettingsController *instance) { s_instance = instance; }

SettingsController *SettingsController::create(QQmlEngine *engine, QJSEngine *scriptEngine) {
  Q_UNUSED(engine)
  Q_UNUSED(scriptEngine)
  Q_ASSERT_X(s_instance, "SettingsController::create", "setInstance() must be called before the QML engine loads");
  QQmlEngine::setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
  return s_instance;
}

} // namespace bl::controllers
