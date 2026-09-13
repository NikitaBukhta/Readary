#ifndef READARY_SERVICES_CHARACTERDTO_HPP
#define READARY_SERVICES_CHARACTERDTO_HPP

#include <QString>
#include <QVariantMap>
#include <QtTypes>

namespace readary::services {

struct CharacterDTO {
  qint64 id = 0;
  QString name;
  QString role;

  static CharacterDTO fromMap(const QVariantMap &data);
};

} // namespace readary::services

#endif // READARY_SERVICES_CHARACTERDTO_HPP
