#include "LanguageDetector.hpp"

#include <algorithm>

using namespace Qt::StringLiterals;

namespace {

constexpr QStringView g_ukrainianOnlyLetters{u"іїєґІЇЄҐ"};

QString cyrillicLanguage(const QString &text) {
  const bool ukrainian = std::ranges::any_of(text, [](const QChar ch) { return g_ukrainianOnlyLetters.contains(ch); });
  return ukrainian ? u"uk"_s : u"ru"_s;
}

} // namespace

namespace readary::api {

TextScript detectScript(const QString &text) {
  int latin = 0;
  int cyrillic = 0;

  for (const QChar ch : text) {
    if (!ch.isLetter()) {
      continue;
    }
    switch (ch.script()) {
    case QChar::Script_Latin:
      ++latin;
      break;
    case QChar::Script_Cyrillic:
      ++cyrillic;
      break;
    default:
      break;
    }
  }

  if (latin == 0 && cyrillic == 0) {
    return TextScript::Unknown;
  }
  return cyrillic > latin ? TextScript::Cyrillic : TextScript::Latin;
}

QString detectLanguage(const QString &text) {
  switch (detectScript(text)) {
  case TextScript::Cyrillic:
    return cyrillicLanguage(text);
  case TextScript::Latin:
    return u"en"_s;
  case TextScript::Unknown:
    break;
  }
  return {};
}

QString detectQueryLanguage(const QString &text) {
  if (detectScript(text) != TextScript::Cyrillic) {
    return {};
  }
  return cyrillicLanguage(text);
}

} // namespace readary::api
