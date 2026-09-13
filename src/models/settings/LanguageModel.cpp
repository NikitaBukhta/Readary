#include "models/settings/LanguageModel.hpp"

#include <QCoreApplication>
#include <QLocale>
#include <QLoggingCategory>
#include <QSettings>
#include <QTranslator>

#include <algorithm>
#include <array>

using Qt::StringLiterals::operator""_L1;
using Qt::StringLiterals::operator""_s;

namespace {
Q_LOGGING_CATEGORY(lcLang, "readary.models.language")

using Code = readary::models::LanguageModel::Code;

struct LanguageInfo {
  QStringView label;
  QStringView localeCode;
};

constexpr auto g_settingsKey = "ui/language"_L1;
constexpr std::array<LanguageInfo, static_cast<size_t>(Code::Count)> g_languageInfo{{
    {.label = u"English", .localeCode = u"en"},
    {.label = u"Русский", .localeCode = u"ru"},
    {.label = u"Українська", .localeCode = u"uk"},
}};

const LanguageInfo *infoFor(Code code) {
  const auto index = static_cast<size_t>(code);
  return index < g_languageInfo.size() ? &g_languageInfo.at(index) : nullptr;
}

QString localeCodeFor(Code code) {
  const LanguageInfo *info = infoFor(code);
  return (info != nullptr ? info->localeCode : g_languageInfo.front().localeCode).toString();
}

QString labelFor(Code code) {
  const LanguageInfo *info = infoFor(code);
  return (info != nullptr ? info->label : g_languageInfo.front().label).toString();
}

} // namespace

namespace readary::models {

LanguageModel::LanguageModel(QObject *parent)
    : QObject{parent}, _current{defaultCode()}, _translator{new QTranslator{this}} {
  const QSettings settings;
  const auto stored = settings.value(g_settingsKey);
  if (stored.isValid()) {
    bool ok = false;
    const int storedCode = stored.toInt(&ok);
    if (ok) {
      _current = clamp(storedCode);
    }
  }
  qCInfo(lcLang) << "LanguageModel initialized, current:" << localeCodeFor(_current);
}

LanguageModel::~LanguageModel() = default;

LanguageModel::Code LanguageModel::current() const { return _current; }

void LanguageModel::setCurrent(Code code) {
  if (_current == code) {
    return;
  }

  _current = code;
  QSettings settings;
  settings.setValue(g_settingsKey, static_cast<int>(_current));

  applyCurrent();
  qCInfo(lcLang) << "Current language changed to" << localeCodeFor(_current);
  emit currentChanged();
}

QList<int> LanguageModel::available() {
  return {static_cast<int>(Code::English), static_cast<int>(Code::Russian), static_cast<int>(Code::Ukrainian)};
}

QString LanguageModel::label(Code code) { return labelFor(code); }

QString LanguageModel::localeCode(Code code) { return localeCodeFor(code); }

void LanguageModel::applyCurrent() {
  QCoreApplication::removeTranslator(_translator);

  const QString locale = localeCodeFor(_current);
  const QString resourcePath = u":/i18n/library_%1.qm"_s.arg(locale);

  if (_translator->load(resourcePath)) {
    QCoreApplication::installTranslator(_translator);
    qCInfo(lcLang) << "Translator loaded:" << resourcePath;
    return;
  }

  qCWarning(lcLang) << "Translator not loaded for" << locale << "(missing" << resourcePath
                    << "). Run `python bootstrap.py translate` to generate translation files.";
}

LanguageModel::Code LanguageModel::defaultCode() {
  const QString uiLang = QLocale().name().left(2).toLower();
  if (uiLang == u"ru"_s) {
    return Code::Russian;
  }
  if (uiLang == u"uk"_s) {
    return Code::Ukrainian;
  }
  return Code::English;
}

LanguageModel::Code LanguageModel::clamp(int raw) {
  const int convertedCode = std::max(0, std::min(raw, static_cast<int>(Code::Count) - 1));
  return static_cast<Code>(convertedCode);
}

} // namespace readary::models
