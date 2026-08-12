#ifndef READARY_API_TRANSLATE_LANGUAGEDETECTOR_HPP
#define READARY_API_TRANSLATE_LANGUAGEDETECTOR_HPP

#include <QString>

#include <cstdint>

namespace readary::api {

enum class TextScript : std::uint8_t { Unknown, Latin, Cyrillic };

TextScript detectScript(const QString &text);
QString detectLanguage(const QString &text);
QString detectQueryLanguage(const QString &text);

} // namespace readary::api

#endif // READARY_API_TRANSLATE_LANGUAGEDETECTOR_HPP
