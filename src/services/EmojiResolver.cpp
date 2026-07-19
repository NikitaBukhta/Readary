#include "EmojiResolver.hpp"

#include <QDirIterator>
#include <QFileInfo>
#include <QLoggingCategory>

using namespace Qt::StringLiterals;

namespace {
Q_LOGGING_CATEGORY(lcEmoji, "readary.services.emoji")

constexpr uint g_kVariationSelector16 = 0xFE0F;
constexpr auto g_kResourceRoot = ":/emoji";
constexpr auto g_kUrlPrefix = "qrc:/emoji/";

} // namespace

namespace readary::services {

EmojiResolver::EmojiResolver(QObject *parent) : QObject{parent} { loadAvailableStems(); }

EmojiResolver *EmojiResolver::create(QQmlEngine * /*engine*/, QJSEngine * /*scriptEngine*/) {
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

  return QLatin1String(g_kUrlPrefix) + key + u".svg"_s;
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
    if (cp == g_kVariationSelector16) {
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
  QDirIterator it(QLatin1String(g_kResourceRoot), {u"*.svg"_s}, QDir::Files);
  while (it.hasNext()) {
    it.next();
    _availableStems.insert(it.fileInfo().completeBaseName());
  }
  qCInfo(lcEmoji) << "EmojiResolver indexed" << _availableStems.size() << "Twemoji stems";
}

} // namespace readary::services
