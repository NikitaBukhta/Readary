#include "CharacterDTO.hpp"

namespace bl::services {

CharacterDTO CharacterDTO::fromMap(const QVariantMap &data) {
  CharacterDTO dto;
  dto.id = data.value("id").toLongLong();
  dto.name = data.value("name").toString();
  dto.role = data.value("role").toString();
  return dto;
}

} // namespace bl::services
