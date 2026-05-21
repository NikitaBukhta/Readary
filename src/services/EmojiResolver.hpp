#ifndef BEELIBRARY_SERVICES_EMOJIRESOLVER_HPP
#define BEELIBRARY_SERVICES_EMOJIRESOLVER_HPP

#include <QObject>
#include <QSet>
#include <QString>
#include <QtQml/qqmlregistration.h>

class QJSEngine;
class QQmlEngine;

namespace bl::services {

// Maps emoji glyph strings (one Unicode codepoint or a ZWJ-joined sequence) to
// a qrc:/emoji/<codepoints>.svg URL pointing at the vendored Twemoji SVG.
//
// Twemoji's filename convention: lowercase hex codepoints joined with `-`,
// with the U+FE0F variation selector stripped.
// Examples: 👋 → 1f44b, ❤️ → 2764, 👨‍👩‍👧 → 1f468-200d-1f469-200d-1f467.
class EmojiResolver : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

public:
  explicit EmojiResolver(QObject *parent = nullptr);

  // Returns "qrc:/emoji/<codepoints>.svg" if the input maps to a bundled
  // Twemoji glyph, otherwise an empty string. Empty input → empty result.
  // Thread-affinity: read-only after construction, safe to call from any
  // thread that owns the QObject's normal access pattern.
  Q_INVOKABLE QString iconUrl(const QString &emoji) const;

  static EmojiResolver *create(QQmlEngine *engine, QJSEngine *scriptEngine);

private:
  static QString resolveKey(const QString &emoji);
  void loadAvailableStems();

  QSet<QString> _availableStems;
};

} // namespace bl::services

#endif // BEELIBRARY_SERVICES_EMOJIRESOLVER_HPP
