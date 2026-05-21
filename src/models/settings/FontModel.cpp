#include "FontModel.hpp"

#include <QFontDatabase>
#include <QLoggingCategory>
#include <QSettings>

#include <map>

using namespace Qt::StringLiterals;

namespace {
Q_LOGGING_CATEGORY(lcFont, "bl.models.font")

struct FontInfo {
  QString label;
  QString resourcePath;
};

constexpr auto g_kSettingsKey = "ui/font";
const std::map<bl::models::FontModel::Code, FontInfo> g_kFontInfoMap{
    {bl::models::FontModel::Code::NotoColorEmoji,
     {u"Noto Color Emoji"_s, u":/fonts/NotoColorEmoji_WindowsCompatible.ttf"_s}},
};

QString labelFor(bl::models::FontModel::Code code) {
  static const QString defaultReturnValue = u"Noto Color Emoji"_s;

  const auto it = g_kFontInfoMap.find(code);
  return it != g_kFontInfoMap.end() ? it->second.label : defaultReturnValue;
}

QString resourcePathFor(bl::models::FontModel::Code code) {
  static const QString defaultReturnValue;

  const auto it = g_kFontInfoMap.find(code);
  return it != g_kFontInfoMap.end() ? it->second.resourcePath : defaultReturnValue;
}

} // namespace

namespace bl::models {

FontModel::FontModel(QObject *parent) : QObject(parent), _current{defaultCode()} {
  const QSettings settings;
  const auto stored = settings.value(g_kSettingsKey);
  if (stored.isValid()) {
    bool ok = false;
    const int v = stored.toInt(&ok);
    if (ok) {
      _current = clamp(v);
    }
  }
  qCInfo(lcFont) << "FontModel initialized, current:" << labelFor(_current);
}

FontModel::~FontModel() = default;

FontModel::Code FontModel::current() const { return _current; }

void FontModel::setCurrent(Code code) {
  if (_current == code)
    return;

  _current = code;
  QSettings settings;
  settings.setValue(g_kSettingsKey, static_cast<int>(_current));

  applyCurrent();
  qCInfo(lcFont) << "Current font changed to" << labelFor(_current);
  emit currentChanged();
}

QString FontModel::currentFamily() const { return _loadedFamilies.value(_current, labelFor(_current)); }

QList<int> FontModel::available() { return {static_cast<int>(Code::NotoColorEmoji)}; }

QString FontModel::label(Code code) { return labelFor(code); }

QString FontModel::resourcePath(Code code) { return resourcePathFor(code); }

QString FontModel::familyName(Code code) const { return _loadedFamilies.value(code, labelFor(code)); }

void FontModel::applyCurrent() {
  // Load the font into QFontDatabase so it is reachable by family name from
  // QML. We intentionally do NOT modify QGuiApplication's default font: on
  // Windows that pushed DirectWrite to try rendering our bundled file instead
  // of Segoe UI Emoji and produced tofu for inline emoji. Consumers that need
  // this font ask for it by family explicitly (e.g. IconGlyph on Android).
  loadFont(_current);
}

QString FontModel::loadFont(Code code) {
  if (_loadedFamilies.contains(code))
    return _loadedFamilies.value(code);

  const QString path = resourcePathFor(code);
  if (path.isEmpty()) {
    qCWarning(lcFont) << "No resource path registered for font code" << static_cast<int>(code);
    return {};
  }

  const int id = QFontDatabase::addApplicationFont(path);
  if (id < 0) {
    qCWarning(lcFont) << "Failed to load font from" << path
                      << "— resource missing or font format unsupported on this platform.";
    return {};
  }

  const QStringList families = QFontDatabase::applicationFontFamilies(id);
  if (families.isEmpty()) {
    qCWarning(lcFont) << "Font loaded from" << path << "but reported no families";
    return {};
  }

  const QString family = families.first();
  _loadedFamilies.insert(code, family);
  qCInfo(lcFont) << "Font loaded:" << path << "as family" << family;
  return family;
}

FontModel::Code FontModel::defaultCode() { return Code::NotoColorEmoji; }

FontModel::Code FontModel::clamp(int raw) {
  const int convertedCode = std::max(0, std::min(raw, static_cast<int>(Code::Count) - 1));
  return static_cast<Code>(convertedCode);
}

} // namespace bl::models
