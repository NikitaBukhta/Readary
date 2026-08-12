#ifndef READARY_API_TRANSLATE_LANGUAGECONVERTER_HPP
#define READARY_API_TRANSLATE_LANGUAGECONVERTER_HPP

#include <QLocale>
#include <QString>

namespace readary::api {

class LanguageConverter {
public:
  static QLocale::Language fromCode(const QString &code);

  static QString toIso639_1(QLocale::Language language);
  static QString toIso639_1(const QString &code);

  static QString toMarc(QLocale::Language language);
  static QString toMarc(const QString &code);
};

} // namespace readary::api

#endif // READARY_API_TRANSLATE_LANGUAGECONVERTER_HPP
