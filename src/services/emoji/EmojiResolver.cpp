#include "services/emoji/EmojiResolver.hpp"

#include <QDirIterator>
#include <QFileInfo>
#include <QLoggingCategory>

using Qt::StringLiterals::operator""_L1;
using Qt::StringLiterals::operator""_s;

namespace {
Q_LOGGING_CATEGORY(lcEmoji, "readary.services.emoji")

constexpr uint g_variationSelector16{0xFE0F};
constexpr auto g_resourceRoot = ":/emoji"_L1;
constexpr auto g_urlPrefix = "qrc:/emoji/"_L1;
constexpr auto g_svgSuffix = ".svg"_L1;

} // namespace

namespace readary::services {

EmojiResolver::EmojiResolver(QObject *parent) : QObject{parent} { loadAvailableStems(); }

EmojiResolver *EmojiResolver::create(QQmlEngine *engine, QJSEngine *scriptEngine) {
  Q_UNUSED(engine)
  Q_UNUSED(scriptEngine)
  return new EmojiResolver{};
}

QString EmojiResolver::iconUrl(const QString &emoji) const {
  if (emoji.isEmpty()) {
    return {};
  }

  const QString key = resolveKey(emoji);
  if (key.isEmpty() || !_availableStems.contains(key)) {
    return {};
  }

  return g_urlPrefix + key + g_svgSuffix;
}

QString EmojiResolver::resolveKey(const QString &emoji) {
  // Build a Twemoji filename stem from the input's codepoints. Twemoji omits
  // the U+FE0F variation selector from filenames in all sequences.
  // Direct write into a pre-sized QString avoids the intermediate QStringList
  // + join allocation that the previous implementation paid on every call.
  const QList<uint> codepoints = emoji.toUcs4();
  if (codepoints.isEmpty()) {
    return {};
  }

  // Codepoints fit in <= 6 hex chars (U+10FFFF), plus a separator. Reserve a
  // little extra so single-codepoint emoji never trigger a reallocation.
  QString result;
  result.reserve(codepoints.size() * 7);
  bool first = true;
  for (const uint cp : codepoints) {
    if (cp == g_variationSelector16) {
      continue;
    }
    if (!first) {
      result.append(u'-');
    }
    result.append(QString::number(cp, 16));
    first = false;
  }
  return result;
}

void EmojiResolver::loadAvailableStems() {
  // Enumerate the qrc once at construction so iconUrl can answer with an
  // O(1) hash lookup rather than a per-call QFile::exists trie walk. The
  // working set is ~4k stems (Twemoji), ~80 KB of QString — negligible.
  _availableStems.reserve(4096);
  QDirIterator it(g_resourceRoot, {u"*.svg"_s}, QDir::Files);
  while (it.hasNext()) {
    it.next();
    _availableStems.insert(it.fileInfo().completeBaseName());
  }
  qCInfo(lcEmoji) << "EmojiResolver indexed" << _availableStems.size() << "Twemoji stems";
}

} // namespace readary::services
