#ifndef READARY_UTILS_ISBNVALIDATOR_HPP
#define READARY_UTILS_ISBNVALIDATOR_HPP

#include <QString>

#include <array>
#include <optional>

namespace readary::utils {

class IsbnValidator {
public:
  static std::optional<qint64> convert(const QString &val);

  // First valid ISBN printed in a block of text, normalised the same way
  // convert() does. Looks only where an ISBN is actually labelled ("ISBN",
  // "ISBN-13:", …): the checksum rejects a false positive, but anchoring on the
  // label keeps a stray order number or phone number from ever being tried.
  static std::optional<qint64> findIn(const QString &text);

private:
  static constexpr int kMaxDigits = 13;
  static constexpr int kCheckX = 10;
  using Digits = std::array<int, kMaxDigits>;

  static std::optional<int> collectDigits(const QString &val, Digits &out);
  static std::optional<qint64> fromIsbn10(const Digits &digits);
  static std::optional<qint64> fromIsbn13(const Digits &digits);
};

} // namespace readary::utils

#endif // READARY_UTILS_ISBNVALIDATOR_HPP
