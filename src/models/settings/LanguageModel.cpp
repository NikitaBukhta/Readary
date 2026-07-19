#include "LanguageModel.hpp"

#include <QCoreApplication>
#include <QLocale>
#include <QLoggingCategory>
#include <QSettings>
#include <QTranslator>

#include <map>

using namespace Qt::StringLiterals;

namespace {
Q_LOGGING_CATEGORY(lcLang, "readary.models.language")

struct LanguageInfo {
  QString label;
  QString localeCode;
};

constexpr auto g_kSettingsKey = "ui/language";
const std::map<readary::models::LanguageModel::Code, LanguageInfo> g_kLanguageInfoMap{
    {readary::models::LanguageModel::Code::English, {.label = u"English"_s, .localeCode = u"en"_s}},
    {readary::models::LanguageModel::Code::Russian, {.label = u"Русский"_s, .localeCode = u"ru"_s}},
    {readary::models::LanguageModel::Code::Ukrainian, {.label = u"Українська"_s, .localeCode = u"uk"_s}},
};

QString localeCodeFor(readary::models::LanguageModel::Code code) {
  static const QString defaultReturnValue = u"en"_s;

  const auto it = g_kLanguageInfoMap.find(code);
  return it != g_kLanguageInfoMap.end() ? it->second.localeCode : defaultReturnValue;
}

QString labelFor(readary::models::LanguageModel::Code code) {
  static const QString defaultReturnValue = u"English"_s;

  const auto it = g_kLanguageInfoMap.find(code);
  return it != g_kLanguageInfoMap.end() ? it->second.label : defaultReturnValue;
}

} // namespace

namespace readary::models {

LanguageModel::LanguageModel(QObject *parent)
    : QObject{parent}, _current{defaultCode()}, _translator{new QTranslator{this}} {
  const QSettings settings;
  const auto stored = settings.value(g_kSettingsKey);
  if (stored.isValid()) {
    bool ok = false;
    const int v = stored.toInt(&ok);
    if (ok) {
      _current = clamp(v);
    }
  }
  qCInfo(lcLang) << "LanguageModel initialized, current:" << localeCodeFor(_current);
}

LanguageModel::~LanguageModel() = default;

LanguageModel::Code LanguageModel::current() const { return _current; }

void LanguageModel::setCurrent(Code code) {
  if (_current == code)
    return;

  _current = code;
  QSettings settings;
  settings.setValue(g_kSettingsKey, static_cast<int>(_current));

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
