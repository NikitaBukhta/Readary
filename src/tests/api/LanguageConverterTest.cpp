#include "api/translate/LanguageConverter.hpp"

#include <QLocale>
#include <QString>
#include <QTest>

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
  QCOMPARE(LanguageConverter::toIso639_1(QStringLiteral("rus")), QStringLiteral("ru"));
  QCOMPARE(LanguageConverter::toIso639_1(QStringLiteral("eng")), QStringLiteral("en"));
  QCOMPARE(LanguageConverter::toIso639_1(QStringLiteral("ukr")), QStringLiteral("uk"));
  QCOMPARE(LanguageConverter::toIso639_1(QStringLiteral("ger")), QStringLiteral("de"));
  QCOMPARE(LanguageConverter::toIso639_1(QStringLiteral("fre")), QStringLiteral("fr"));
}

void LanguageConverterTest::iso639_1ToMarc_common() {
  QCOMPARE(LanguageConverter::toMarc(QStringLiteral("ru")), QStringLiteral("rus"));
  QCOMPARE(LanguageConverter::toMarc(QStringLiteral("en")), QStringLiteral("eng"));
  QCOMPARE(LanguageConverter::toMarc(QStringLiteral("uk")), QStringLiteral("ukr"));
  QCOMPARE(LanguageConverter::toMarc(QStringLiteral("de")), QStringLiteral("ger"));
  QCOMPARE(LanguageConverter::toMarc(QStringLiteral("fr")), QStringLiteral("fre"));
}

void LanguageConverterTest::roundTrip_marcToIsoToMarc() {
  const QString iso = LanguageConverter::toIso639_1(QStringLiteral("spa"));
  QCOMPARE(iso, QStringLiteral("es"));
  QCOMPARE(LanguageConverter::toMarc(iso), QStringLiteral("spa"));
}

void LanguageConverterTest::iso639_1_passesThroughToIso() {
  QCOMPARE(LanguageConverter::toIso639_1(QStringLiteral("ru")), QStringLiteral("ru"));
}

void LanguageConverterTest::unknownCode_returnsEmpty() {
  QVERIFY(LanguageConverter::toIso639_1(QStringLiteral("zzz")).isEmpty());
  QVERIFY(LanguageConverter::toMarc(QStringLiteral("zzz")).isEmpty());
}

void LanguageConverterTest::languageWithoutIso639_1_returnsEmpty() {
  // Tuvan has a MARC/639-2 code but no ISO 639-1 code → should drop out.
  QVERIFY(LanguageConverter::toIso639_1(QStringLiteral("tyv")).isEmpty());
}

void LanguageConverterTest::fromCode_resolvesLanguage() {
  QCOMPARE(LanguageConverter::fromCode(QStringLiteral("rus")), QLocale::Russian);
  QCOMPARE(LanguageConverter::fromCode(QStringLiteral("en")), QLocale::English);
  QCOMPARE(LanguageConverter::fromCode(QStringLiteral("zzz")), QLocale::AnyLanguage);
}

QTEST_GUILESS_MAIN(LanguageConverterTest)
#include "LanguageConverterTest.moc"
