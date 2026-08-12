#ifndef LIBRARY_ITRANSLATOR_HPP
#define LIBRARY_ITRANSLATOR_HPP

#include <QObject>
#include <QString>

namespace readary::api {

class ITranslator : public QObject {
  Q_OBJECT
public:
  explicit ITranslator(QObject *parent = nullptr) : QObject{parent} {}
  ~ITranslator() override = default;

  virtual void translate(const QString &text, const QString &targetLang, quint64 requestId) = 0;

signals:
  void translationReady(quint64 requestId, QString translated);
};

} // namespace readary::api

#endif // LIBRARY_ITRANSLATOR_HPP
