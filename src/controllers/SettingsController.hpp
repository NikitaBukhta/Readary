#ifndef BEELIBRARY_CONTROLLERS_SETTINGSCONTROLLER_HPP
#define BEELIBRARY_CONTROLLERS_SETTINGSCONTROLLER_HPP

#include "models/settings/FontModel.hpp"
#include "models/settings/LanguageModel.hpp"

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>

namespace readary::controllers {

class SettingsController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(readary::models::LanguageModel *languageModel READ languageModel CONSTANT FINAL)
  Q_PROPERTY(readary::models::FontModel *fontModel READ fontModel CONSTANT FINAL)

public:
  explicit SettingsController(QObject *parent = nullptr);
  ~SettingsController() override;

  readary::models::LanguageModel *languageModel() const;
  readary::models::FontModel *fontModel() const;

  static SettingsController *create(QQmlEngine *engine, QJSEngine *scriptEngine);
  static void setInstance(SettingsController *instance);

private:
  static SettingsController *s_instance;

  readary::models::LanguageModel *_languageModel;
  readary::models::FontModel *_fontModel;
};

} // namespace readary::controllers

#endif // BEELIBRARY_CONTROLLERS_SETTINGSCONTROLLER_HPP
