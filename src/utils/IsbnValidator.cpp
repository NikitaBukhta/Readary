#include "IsbnValidator.hpp"

namespace readary::utils {

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
