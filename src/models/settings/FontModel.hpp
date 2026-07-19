#ifndef BEELIBRARY_MODELS_FONTMODEL_HPP
#define BEELIBRARY_MODELS_FONTMODEL_HPP

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace readary::models {

class FontModel : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("FontModel is owned by SettingsController")

  Q_PROPERTY(Code current READ current WRITE setCurrent NOTIFY currentChanged FINAL)
  Q_PROPERTY(QList<int> available READ available CONSTANT FINAL)
  Q_PROPERTY(QString currentFamily READ currentFamily NOTIFY currentChanged FINAL)

public:
  enum class Code { NotoColorEmoji = 0, Count };
  Q_ENUM(Code)

  explicit FontModel(QObject *parent = nullptr);
  ~FontModel() override;

  Code current() const;
  void setCurrent(Code code);

  QString currentFamily() const;

  static QList<int> available();
  Q_INVOKABLE static QString label(Code code);
  Q_INVOKABLE static QString resourcePath(Code code);
  Q_INVOKABLE QString familyName(Code code) const;

  void applyCurrent();

signals:
  void currentChanged();

private:
  static Code defaultCode();
  static Code clamp(int raw);

  QString loadFont(Code code);

  Code _current;
  QHash<Code, QString> _loadedFamilies;
};

} // namespace readary::models

#endif // BEELIBRARY_MODELS_FONTMODEL_HPP
