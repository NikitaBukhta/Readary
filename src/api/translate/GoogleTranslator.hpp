#ifndef READARY_API_TRANSLATE_GOOGLETRANSLATOR_HPP
#define READARY_API_TRANSLATE_GOOGLETRANSLATOR_HPP

#include "api/translate/ITranslator.hpp"

#include <QHash>
#include <QNetworkAccessManager>

class QNetworkReply;

namespace readary::api {

class GoogleTranslator : public ITranslator {
  Q_OBJECT
public:
  explicit GoogleTranslator(QObject *parent = nullptr);

  void setEndpoint(const QString &endpoint);
  void translate(const QString &text, const QString &targetLang, quint64 requestId) override;

private:
  void onReplyFinished(QNetworkReply *reply);

  QNetworkAccessManager _networkManager;
  QHash<QNetworkReply *, quint64> _requests;
  QString _endpoint;
};

} // namespace readary::api

#endif // READARY_API_TRANSLATE_GOOGLETRANSLATOR_HPP
