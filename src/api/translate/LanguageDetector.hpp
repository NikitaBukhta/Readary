#ifndef LIBRARY_LANGUAGEDETECTOR_HPP
#define LIBRARY_LANGUAGEDETECTOR_HPP

#include <QString>

namespace readary::api {

enum class TextScript : uint8_t { Unknown, Latin, Cyrillic };

TextScript detectScript(const QString &text);
QString detectLanguage(const QString &text);
QString detectQueryLanguage(const QString &text);

} // namespace readary::api

#endif // LIBRARY_LANGUAGEDETECTOR_HPP
