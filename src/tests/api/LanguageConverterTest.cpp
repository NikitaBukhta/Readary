#include "api/translate/LanguageConverter.hpp"

#include <QLocale>
#include <QString>
#include <QTest>

using Qt::StringLiterals::operator""_s;

using readary::api::LanguageConverter;

class LanguageConverterTest : public QObject {
  Q_OBJECT

private slots:
  void marcToIso639_1_common();
  void iso639_1ToMarc_common();
  void roundTrip_marcToIsoToMarc();
  void iso639_1_passesThroughToIso();
  void unknownCode_returnsEmpty();
  void languageWithoutIso639_1_returnsEmpty();
  void fromCode_resolvesLanguage();
};

void LanguageConverterTest::marcToIso639_1_common() {
  QCOMPARE(LanguageConverter::toIso639_1(u"rus"_s), u"ru"_s);
  QCOMPARE(LanguageConverter::toIso639_1(u"eng"_s), u"en"_s);
  QCOMPARE(LanguageConverter::toIso639_1(u"ukr"_s), u"uk"_s);
  QCOMPARE(LanguageConverter::toIso639_1(u"ger"_s), u"de"_s);
  QCOMPARE(LanguageConverter::toIso639_1(u"fre"_s), u"fr"_s);
}

void LanguageConverterTest::iso639_1ToMarc_common() {
  QCOMPARE(LanguageConverter::toMarc(u"ru"_s), u"rus"_s);
  QCOMPARE(LanguageConverter::toMarc(u"en"_s), u"eng"_s);
  QCOMPARE(LanguageConverter::toMarc(u"uk"_s), u"ukr"_s);
  QCOMPARE(LanguageConverter::toMarc(u"de"_s), u"ger"_s);
  QCOMPARE(LanguageConverter::toMarc(u"fr"_s), u"fre"_s);
}

void LanguageConverterTest::roundTrip_marcToIsoToMarc() {
  const QString iso = LanguageConverter::toIso639_1(u"spa"_s);
  QCOMPARE(iso, u"es"_s);
  QCOMPARE(LanguageConverter::toMarc(iso), u"spa"_s);
}

void LanguageConverterTest::iso639_1_passesThroughToIso() { QCOMPARE(LanguageConverter::toIso639_1(u"ru"_s), u"ru"_s); }

void LanguageConverterTest::unknownCode_returnsEmpty() {
  QVERIFY(LanguageConverter::toIso639_1(u"zzz"_s).isEmpty());
  QVERIFY(LanguageConverter::toMarc(u"zzz"_s).isEmpty());
}

void LanguageConverterTest::languageWithoutIso639_1_returnsEmpty() {
  // Tuvan has a MARC/639-2 code but no ISO 639-1 code → should drop out.
  QVERIFY(LanguageConverter::toIso639_1(u"tyv"_s).isEmpty());
}

void LanguageConverterTest::fromCode_resolvesLanguage() {
  QCOMPARE(LanguageConverter::fromCode(u"rus"_s), QLocale::Russian);
  QCOMPARE(LanguageConverter::fromCode(u"en"_s), QLocale::English);
  QCOMPARE(LanguageConverter::fromCode(u"zzz"_s), QLocale::AnyLanguage);
}

QTEST_GUILESS_MAIN(LanguageConverterTest)
#include "LanguageConverterTest.moc"
