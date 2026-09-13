#include "utils/IsbnValidator.hpp"

#include <QString>
#include <QTest>

#include <optional>

using Qt::StringLiterals::operator""_s;

using readary::utils::IsbnValidator;

class IsbnValidatorTest : public QObject {
  Q_OBJECT

private slots:
  void convert_data();
  void convert();

  void findIn_readsALabelledIsbn13();
  void findIn_readsALabelledIsbn10AndNormalisesIt();
  void findIn_acceptsTheIsbn13Qualifier();
  void findIn_isCaseInsensitive();
  void findIn_skipsANumberThatFailsItsChecksum();
  void findIn_isbn10_followedByThePrintingNumberLine();
  void findIn_isbn13_followedByThePrintingNumberLine();
  void findIn_ignoresUnlabelledNumbers();
  void findIn_textWithoutAnIsbn_findsNothing();
  void findIn_emptyText_findsNothing();
};

void IsbnValidatorTest::convert_data() {
  QTest::addColumn<QString>("input");
  QTest::addColumn<bool>("valid");
  QTest::addColumn<qint64>("expected");

  // --- ISBN-13: returned as-is when the checksum holds ------------------
  QTest::newRow("isbn13 plain") << u"9780306406157"_s << true << Q_INT64_C(9780306406157);
  QTest::newRow("isbn13 hyphenated") << u"978-0-306-40615-7"_s << true << Q_INT64_C(9780306406157);
  QTest::newRow("isbn13 spaced") << u"978 0 306 40615 7"_s << true << Q_INT64_C(9780306406157);
  QTest::newRow("isbn13 mixed separators") << u"978-0 306-40615 7"_s << true << Q_INT64_C(9780306406157);

  // --- ISBN-10: normalized to its equivalent ISBN-13 value --------------
  QTest::newRow("isbn10 plain") << u"0306406152"_s << true << Q_INT64_C(9780306406157);
  QTest::newRow("isbn10 hyphenated") << u"0-306-40615-2"_s << true << Q_INT64_C(9780306406157);
  QTest::newRow("isbn10 spaced") << u"0 306 40615 2"_s << true << Q_INT64_C(9780306406157);

  // ISBN-10 whose check digit is 'X' (value 10): 0-8044-2957-X.
  QTest::newRow("isbn10 trailing X") << u"080442957X"_s << true << Q_INT64_C(9780804429573);
  QTest::newRow("isbn10 trailing lowercase x") << u"080442957x"_s << true << Q_INT64_C(9780804429573);
  QTest::newRow("isbn10 hyphenated X") << u"0-8044-2957-X"_s << true << Q_INT64_C(9780804429573);

  // --- Invalid input: must yield nullopt --------------------------------
  QTest::newRow("isbn13 bad checksum") << u"9780306406158"_s << false << Q_INT64_C(0);
  QTest::newRow("isbn10 bad checksum") << u"0306406153"_s << false << Q_INT64_C(0);
  QTest::newRow("too short") << u"12345"_s << false << Q_INT64_C(0);
  QTest::newRow("eleven digits") << u"12345678901"_s << false << Q_INT64_C(0);
  QTest::newRow("fourteen digits") << u"97803064061570"_s << false << Q_INT64_C(0);
  QTest::newRow("letters") << u"abcdefghij"_s << false << Q_INT64_C(0);
  QTest::newRow("free-text query") << u"Harry Potter"_s << false << Q_INT64_C(0);
  QTest::newRow("empty") << QString() << false << Q_INT64_C(0);
  QTest::newRow("separators only") << u"----"_s << false << Q_INT64_C(0);
  // 'X' is only legal as the trailing ISBN-10 check digit.
  QTest::newRow("isbn10 X not trailing") << u"X306406152"_s << false << Q_INT64_C(0);
  QTest::newRow("isbn13 contains X") << u"978030640615X"_s << false << Q_INT64_C(0);
}

void IsbnValidatorTest::convert() {
  QFETCH(QString, input);
  QFETCH(bool, valid);
  QFETCH(qint64, expected);

  const std::optional<qint64> result = IsbnValidator::convert(input);

  QCOMPARE(result.has_value(), valid);
  if (valid) {
    QCOMPARE(*result, expected);
  }
}

void IsbnValidatorTest::findIn_readsALabelledIsbn13() {
  const auto found = IsbnValidator::findIn(u"Printed in the USA. ISBN 978-0-13-235088-4. All rights reserved."_s);

  QVERIFY(found.has_value());
  QCOMPARE(*found, 9780132350884LL);
}

void IsbnValidatorTest::findIn_readsALabelledIsbn10AndNormalisesIt() {
  // Same normalisation convert() applies, so a book found by its ISBN-10 keys
  // the same row as the catalog's ISBN-13.
  const auto found = IsbnValidator::findIn(u"ISBN 0-13-235088-2"_s);

  QVERIFY(found.has_value());
  QCOMPARE(*found, 9780132350884LL);
}

void IsbnValidatorTest::findIn_acceptsTheIsbn13Qualifier() {
  // "ISBN-13:" must not have its "13" read as the first two digits.
  const auto found = IsbnValidator::findIn(u"ISBN-13: 9780132350884"_s);

  QVERIFY(found.has_value());
  QCOMPARE(*found, 9780132350884LL);
}

void IsbnValidatorTest::findIn_isCaseInsensitive() {
  QCOMPARE(IsbnValidator::findIn(u"isbn 9780132350884"_s), std::optional<qint64>{9780132350884LL});
}

void IsbnValidatorTest::findIn_skipsANumberThatFailsItsChecksum() {
  // A copyright page can carry several editions; only a valid one is taken.
  const auto found = IsbnValidator::findIn(u"ISBN 978-0-13-235088-9 (ebook) ISBN 978-0-13-235088-4 (print)"_s);

  QVERIFY(found.has_value());
  QCOMPARE(*found, 9780132350884LL);
}

void IsbnValidatorTest::findIn_isbn10_followedByThePrintingNumberLine() {
  // Copyright pages almost always print the descending printing number under the
  // ISBN. Those digits must not be swallowed into the ISBN being read.
  const auto found = IsbnValidator::findIn(u"ISBN 0-306-40615-2\n10 9 8 7 6 5 4 3 2 1"_s);

  QVERIFY(found.has_value());
  QCOMPARE(*found, 9780306406157LL);
}

void IsbnValidatorTest::findIn_isbn13_followedByThePrintingNumberLine() {
  const auto found = IsbnValidator::findIn(u"ISBN 978-0-13-235088-4\n10 9 8 7 6 5 4 3 2 1"_s);

  QVERIFY(found.has_value());
  QCOMPARE(*found, 9780132350884LL);
}

void IsbnValidatorTest::findIn_ignoresUnlabelledNumbers() {
  // Anchored on the label so an order number or a phone number is never tried.
  QVERIFY(!IsbnValidator::findIn(u"Order 9780132350884 by phone on 555 0132 350884"_s).has_value());
}

void IsbnValidatorTest::findIn_textWithoutAnIsbn_findsNothing() {
  QVERIFY(!IsbnValidator::findIn(u"Chapter one. It was a bright cold day in April."_s).has_value());
}

void IsbnValidatorTest::findIn_emptyText_findsNothing() { QVERIFY(!IsbnValidator::findIn({}).has_value()); }

QTEST_GUILESS_MAIN(IsbnValidatorTest)
#include "IsbnValidatorTest.moc"
