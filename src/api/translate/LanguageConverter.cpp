#include "LanguageConverter.hpp"

namespace readary::api {

QLocale::Language LanguageConverter::fromCode(const QString &code) {
  return QLocale::codeToLanguage(code, QLocale::ISO639Part1 | QLocale::ISO639Part2B | QLocale::ISO639Part2T |
                                           QLocale::ISO639Part3);
}

QString LanguageConverter::toIso639_1(QLocale::Language language) {
  if (language == QLocale::AnyLanguage) {
    return {};
  }
  return QLocale::languageToCode(language, QLocale::ISO639Part1);
}

QString LanguageConverter::toIso639_1(const QString &code) { return toIso639_1(fromCode(code)); }

QString LanguageConverter::toMarc(QLocale::Language language) {
  if (language == QLocale::AnyLanguage) {
    return {};
  }
  return QLocale::languageToCode(language, QLocale::ISO639Part2B);
}

QString LanguageConverter::toMarc(const QString &code) { return toMarc(fromCode(code)); }

} // namespace readary::api
