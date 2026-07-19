#include "utils/IsbnValidator.hpp"

#include <QString>
#include <QTest>

#include <optional>

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
  QTest::newRow("isbn13 plain") << QStringLiteral("9780306406157") << true << Q_INT64_C(9780306406157);
  QTest::newRow("isbn13 hyphenated") << QStringLiteral("978-0-306-40615-7") << true << Q_INT64_C(9780306406157);
  QTest::newRow("isbn13 spaced") << QStringLiteral("978 0 306 40615 7") << true << Q_INT64_C(9780306406157);
  QTest::newRow("isbn13 mixed separators") << QStringLiteral("978-0 306-40615 7") << true << Q_INT64_C(9780306406157);

  // --- ISBN-10: normalized to its equivalent ISBN-13 value --------------
  QTest::newRow("isbn10 plain") << QStringLiteral("0306406152") << true << Q_INT64_C(9780306406157);
  QTest::newRow("isbn10 hyphenated") << QStringLiteral("0-306-40615-2") << true << Q_INT64_C(9780306406157);
  QTest::newRow("isbn10 spaced") << QStringLiteral("0 306 40615 2") << true << Q_INT64_C(9780306406157);

  // ISBN-10 whose check digit is 'X' (value 10): 0-8044-2957-X.
  QTest::newRow("isbn10 trailing X") << QStringLiteral("080442957X") << true << Q_INT64_C(9780804429573);
  QTest::newRow("isbn10 trailing lowercase x") << QStringLiteral("080442957x") << true << Q_INT64_C(9780804429573);
  QTest::newRow("isbn10 hyphenated X") << QStringLiteral("0-8044-2957-X") << true << Q_INT64_C(9780804429573);

  // --- Invalid input: must yield nullopt --------------------------------
  QTest::newRow("isbn13 bad checksum") << QStringLiteral("9780306406158") << false << Q_INT64_C(0);
  QTest::newRow("isbn10 bad checksum") << QStringLiteral("0306406153") << false << Q_INT64_C(0);
  QTest::newRow("too short") << QStringLiteral("12345") << false << Q_INT64_C(0);
  QTest::newRow("eleven digits") << QStringLiteral("12345678901") << false << Q_INT64_C(0);
  QTest::newRow("fourteen digits") << QStringLiteral("97803064061570") << false << Q_INT64_C(0);
  QTest::newRow("letters") << QStringLiteral("abcdefghij") << false << Q_INT64_C(0);
  QTest::newRow("free-text query") << QStringLiteral("Harry Potter") << false << Q_INT64_C(0);
  QTest::newRow("empty") << QString() << false << Q_INT64_C(0);
  QTest::newRow("separators only") << QStringLiteral("----") << false << Q_INT64_C(0);
  // 'X' is only legal as the trailing ISBN-10 check digit.
  QTest::newRow("isbn10 X not trailing") << QStringLiteral("X306406152") << false << Q_INT64_C(0);
  QTest::newRow("isbn13 contains X") << QStringLiteral("978030640615X") << false << Q_INT64_C(0);
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
