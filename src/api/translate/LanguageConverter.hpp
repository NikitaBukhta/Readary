#ifndef LIBRARY_LANGUAGECONVERTER_HPP
#define LIBRARY_LANGUAGECONVERTER_HPP

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

#endif // LIBRARY_LANGUAGECONVERTER_HPP
