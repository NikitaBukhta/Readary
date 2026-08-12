#ifndef READARY_UTILS_ISBNVALIDATOR_HPP
#define READARY_UTILS_ISBNVALIDATOR_HPP

#include <QString>

#include <array>
#include <optional>

namespace readary::utils {

class IsbnValidator {
public:
  static std::optional<qint64> convert(const QString &val);

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
