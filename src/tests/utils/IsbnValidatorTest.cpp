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

QTEST_GUILESS_MAIN(IsbnValidatorTest)
#include "IsbnValidatorTest.moc"
