#include "CharacterDTO.hpp"

using Qt::StringLiterals::operator""_s;

namespace readary::services {

CharacterDTO CharacterDTO::fromMap(const QVariantMap &data) {
  CharacterDTO dto;
  dto.id = data.value(u"id"_s).toLongLong();
  dto.name = data.value(u"name"_s).toString();
  dto.role = data.value(u"role"_s).toString();
  return dto;
}

} // namespace readary::services
