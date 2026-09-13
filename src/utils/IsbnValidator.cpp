#include "utils/IsbnValidator.hpp"

#include <QRegularExpression>

namespace {
// The first `count` significant characters of a run, or empty when it holds
// fewer. Separators are dropped here so the caller can cut the run to an exact
// ISBN length; convert() tolerates them either way.
QString firstSignificant(const QString &run, int count) {
  QString out;
  out.reserve(count);
  for (const QChar ch : run) {
    if (!ch.isDigit() && ch != u'X' && ch != u'x') {
      continue;
    }
    out += ch;
    if (out.size() == count) {
      return out;
    }
  }
  return {};
}
} // namespace

namespace readary::utils {

std::optional<qint64> IsbnValidator::findIn(const QString &text) {
  // "ISBN 978-0-13-235088-4", "ISBN-13: 9780132350884", "isbn 0132350882".
  // A raw literal so the regex escapes read as themselves. The run is captured
  // generously — copyright pages print the descending printing number right
  // under the ISBN, and a greedy match would otherwise swallow it.
  static const QRegularExpression pattern{QStringLiteral(R"(ISBN(?:[\s-]*1[03])?[\s:]*((?:[0-9Xx][\s-]*){9,19}))"),
                                          QRegularExpression::CaseInsensitiveOption};

  QRegularExpressionMatchIterator matches = pattern.globalMatch(text);
  while (matches.hasNext()) {
    const QString run = matches.next().captured(1);
    // An ISBN is 13 or 10 significant characters; cut the run to each in turn
    // rather than trusting where it happened to end.
    for (const int length : {kMaxDigits, 10}) {
      // A book often prints both its ISBN-10 and ISBN-13, and back matter can
      // carry other editions' — the first that passes its checksum wins.
      if (const auto isbn = convert(firstSignificant(run, length))) {
        return isbn;
      }
    }
  }
  return std::nullopt;
}

std::optional<qint64> IsbnValidator::convert(const QString &val) {
  Digits digits{};
  const auto count = collectDigits(val, digits);
  if (!count) {
    return std::nullopt;
  }

  if (*count == 10) {
    return fromIsbn10(digits);
  }
  if (*count == kMaxDigits) {
    return fromIsbn13(digits);
  }
  return std::nullopt;
}

std::optional<int> IsbnValidator::collectDigits(const QString &val, Digits &out) {
  int count = 0;
  for (const QChar ch : val) {
    if (ch == u'-' || ch == u' ') {
      continue;
    }
    if (count >= kMaxDigits) {
      return std::nullopt; // More significant characters than any ISBN allows.
    }
    if (ch.isDigit()) {
      out.at(count) = ch.digitValue();
    } else if (ch == u'X' || ch == u'x') {
      out.at(count) = kCheckX; // Only valid as an ISBN-10 check digit; checked later.
    } else {
      return std::nullopt;
    }
    ++count;
  }
  return count;
}

std::optional<qint64> IsbnValidator::fromIsbn10(const Digits &digits) {
  int sum = 0;
  for (int i = 0; i < 10; ++i) {
    const int value = digits.at(i);
    // 'X' (kCheckX) is only permitted as the trailing check digit.
    if (value == kCheckX && i != 9) {
      return std::nullopt;
    }
    sum += value * (10 - i);
  }
  if (sum % 11 != 0) {
    return std::nullopt;
  }

  // Normalize to ISBN-13: prefix 978, the first 9 digits, then a new check digit.
  qint64 value = 978;
  int checkSum = (9 * 1) + (7 * 3) + (8 * 1); // weights for the "978" prefix
  for (int i = 0; i < 9; ++i) {
    const int digit = digits.at(i);
    value = (value * 10) + digit;
    checkSum += digit * ((i % 2 == 0) ? 3 : 1); // prefix occupies indices 0-2
  }
  const int checkDigit = (10 - (checkSum % 10)) % 10;
  return (value * 10) + checkDigit;
}

std::optional<qint64> IsbnValidator::fromIsbn13(const Digits &digits) {
  qint64 value = 0;
  int sum = 0;
  for (int i = 0; i < kMaxDigits; ++i) {
    const int digit = digits.at(i);
    if (digit == kCheckX) {
      return std::nullopt; // 'X' is not a valid ISBN-13 character.
    }
    sum += digit * ((i % 2 == 0) ? 1 : 3);
    value = (value * 10) + digit;
  }
  if (sum % 10 != 0) {
    return std::nullopt;
  }
  return value;
}

} // namespace readary::utils
