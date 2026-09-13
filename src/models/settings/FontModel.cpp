#include "models/settings/FontModel.hpp"

#include <QFontDatabase>
#include <QLoggingCategory>
#include <QSettings>

#include <algorithm>
#include <array>

using Qt::StringLiterals::operator""_L1;

namespace {
Q_LOGGING_CATEGORY(lcFont, "readary.models.font")

using Code = readary::models::FontModel::Code;

struct FontInfo {
  QStringView label;
  QStringView resourcePath;
};

constexpr auto g_settingsKey = "ui/font"_L1;
constexpr std::array<FontInfo, static_cast<size_t>(Code::Count)> g_fontInfo{{
    {.label = u"Noto Color Emoji", .resourcePath = u":/fonts/NotoColorEmoji_WindowsCompatible.ttf"},
}};

const FontInfo *infoFor(Code code) {
  const auto index = static_cast<size_t>(code);
  return index < g_fontInfo.size() ? &g_fontInfo.at(index) : nullptr;
}

QString labelFor(Code code) {
  const FontInfo *info = infoFor(code);
  return (info != nullptr ? info->label : g_fontInfo.front().label).toString();
}

QString resourcePathFor(Code code) {
  const FontInfo *info = infoFor(code);
  return info != nullptr ? info->resourcePath.toString() : QString{};
}

} // namespace

namespace readary::models {

FontModel::FontModel(QObject *parent) : QObject{parent}, _current{defaultCode()} {
  const QSettings settings;
  const auto stored = settings.value(g_settingsKey);
  if (stored.isValid()) {
    bool ok = false;
    const int storedCode = stored.toInt(&ok);
    if (ok) {
      _current = clamp(storedCode);
    }
  }
  qCInfo(lcFont) << "FontModel initialized, current:" << labelFor(_current);
}

FontModel::~FontModel() = default;

FontModel::Code FontModel::current() const { return _current; }

void FontModel::setCurrent(Code code) {
  if (_current == code) {
    return;
  }

  _current = code;
  QSettings settings;
  settings.setValue(g_settingsKey, static_cast<int>(_current));

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
  if (_loadedFamilies.contains(code)) {
    return _loadedFamilies.value(code);
  }

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

  const QString &family = families.first();
  _loadedFamilies.insert(code, family);
  qCInfo(lcFont) << "Font loaded:" << path << "as family" << family;
  return family;
}

FontModel::Code FontModel::defaultCode() { return Code::NotoColorEmoji; }

FontModel::Code FontModel::clamp(int raw) {
  const int convertedCode = std::max(0, std::min(raw, static_cast<int>(Code::Count) - 1));
  return static_cast<Code>(convertedCode);
}

} // namespace readary::models
