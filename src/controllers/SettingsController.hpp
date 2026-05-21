#ifndef BEELIBRARY_CONTROLLERS_SETTINGSCONTROLLER_HPP
#define BEELIBRARY_CONTROLLERS_SETTINGSCONTROLLER_HPP

#include "models/settings/FontModel.hpp"
#include "models/settings/LanguageModel.hpp"

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>

namespace bl::controllers {

class SettingsController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(bl::models::LanguageModel *languageModel READ languageModel CONSTANT FINAL)
  Q_PROPERTY(bl::models::FontModel *fontModel READ fontModel CONSTANT FINAL)

public:
  explicit SettingsController(QObject *parent = nullptr);
  ~SettingsController() override;

  bl::models::LanguageModel *languageModel() const;
  bl::models::FontModel *fontModel() const;

  static SettingsController *create(QQmlEngine *engine, QJSEngine *scriptEngine);
  static void setInstance(SettingsController *instance);

private:
  static SettingsController *s_instance;

  bl::models::LanguageModel *_languageModel;
  bl::models::FontModel *_fontModel;
};

} // namespace bl::controllers

#endif // BEELIBRARY_CONTROLLERS_SETTINGSCONTROLLER_HPP
