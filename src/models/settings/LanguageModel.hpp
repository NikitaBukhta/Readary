#ifndef BEELIBRARY_MODELS_LANGUAGEMODEL_HPP
#define BEELIBRARY_MODELS_LANGUAGEMODEL_HPP

#include <QList>
#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

class QTranslator;

namespace bl::models {

class LanguageModel : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("LanguageModel is owned by SettingsController")

  Q_PROPERTY(Code current READ current WRITE setCurrent NOTIFY currentChanged FINAL)
  Q_PROPERTY(QList<int> available READ available CONSTANT FINAL)

public:
  enum class Code { English = 0, Russian = 1, Ukrainian = 2, Count };
  Q_ENUM(Code)

  explicit LanguageModel(QObject *parent = nullptr);
  ~LanguageModel() override;

  Code current() const;
  void setCurrent(Code code);

  static QList<int> available();
  Q_INVOKABLE static QString label(Code code);
  Q_INVOKABLE static QString localeCode(Code code);

  void applyCurrent();

signals:
  void currentChanged();

private:
  static Code defaultCode();
  static Code clamp(int raw);

  Code _current;
  QTranslator *_translator;
};

} // namespace bl::models

#endif // BEELIBRARY_MODELS_LANGUAGEMODEL_HPP
