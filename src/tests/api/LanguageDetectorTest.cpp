#include "api/translate/LanguageDetector.hpp"

#include <QString>
#include <QTest>

using Qt::StringLiterals::operator""_s;

using readary::api::detectLanguage;
using readary::api::detectQueryLanguage;
using readary::api::detectScript;
using readary::api::TextScript;

class LanguageDetectorTest : public QObject {
  Q_OBJECT

private slots:
  void detectScript_classifiesByDominantScript();
  void detectLanguage_cyrillicSplitsRussianFromUkrainian();
  void detectLanguage_latinResolvesToEnglish();
  void detectLanguage_withoutLettersIsEmpty();
  void detectQueryLanguage_onlyFiltersOnCyrillic();
};

void LanguageDetectorTest::detectScript_classifiesByDominantScript() {
  QCOMPARE(detectScript(u"War and Peace"_s), TextScript::Latin);
  QCOMPARE(detectScript(u"Война и мир"_s), TextScript::Cyrillic);
  QCOMPARE(detectScript(u"1984 (Orwell)"_s), TextScript::Latin);
  QCOMPARE(detectScript(u"Толстой, 1869"_s), TextScript::Cyrillic);
  QCOMPARE(detectScript(u"Гарри Поттер HD"_s), TextScript::Cyrillic);
}

void LanguageDetectorTest::detectLanguage_cyrillicSplitsRussianFromUkrainian() {
  QCOMPARE(detectLanguage(u"Война и мир"_s), u"ru"_s);
  QCOMPARE(detectLanguage(u"Місто"_s), u"uk"_s);  // і
  QCOMPARE(detectLanguage(u"Їжак"_s), u"uk"_s);   // Ї
  QCOMPARE(detectLanguage(u"Європа"_s), u"uk"_s); // Є
  QCOMPARE(detectLanguage(u"ґанок"_s), u"uk"_s);  // ґ
  QCOMPARE(detectLanguage(u"Кобзар Тараса Шевченка"_s), u"ru"_s);
}

void LanguageDetectorTest::detectLanguage_latinResolvesToEnglish() {
  QCOMPARE(detectLanguage(u"War and Peace"_s), u"en"_s);
  QCOMPARE(detectLanguage(u"Der Steppenwolf"_s), u"en"_s);
}

void LanguageDetectorTest::detectLanguage_withoutLettersIsEmpty() {
  QCOMPARE(detectLanguage(QString{}), QString{});
  QCOMPARE(detectLanguage(u"   "_s), QString{});
  QCOMPARE(detectLanguage(u"9780306406157"_s), QString{});
  QCOMPARE(detectScript(u"1234 !?"_s), TextScript::Unknown);
}

void LanguageDetectorTest::detectQueryLanguage_onlyFiltersOnCyrillic() {
  QCOMPARE(detectQueryLanguage(u"Война и мир"_s), u"ru"_s);
  QCOMPARE(detectQueryLanguage(u"Місто"_s), u"uk"_s);
  QCOMPARE(detectQueryLanguage(u"War and Peace"_s), QString{});
  QCOMPARE(detectQueryLanguage(QString{}), QString{});
}

QTEST_GUILESS_MAIN(LanguageDetectorTest)
#include "LanguageDetectorTest.moc"
