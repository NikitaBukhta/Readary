#ifndef BEELIBRARY_SERVICES_CHARACTERDTO_HPP
#define BEELIBRARY_SERVICES_CHARACTERDTO_HPP

#include <QString>
#include <QVariantMap>
#include <QtTypes>

namespace bl::services {

struct CharacterDTO {
  qint64 id = 0;
  QString name;
  QString role;

  static CharacterDTO fromMap(const QVariantMap &data);
};

} // namespace bl::services

#endif // BEELIBRARY_SERVICES_CHARACTERDTO_HPP
