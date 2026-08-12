#ifndef READARY_SERVICES_BOOKSTATUS_HPP
#define READARY_SERVICES_BOOKSTATUS_HPP

#include <QObject>
#include <QtQml/qqmlregistration.h>

namespace readary::services {

// QML-visible enum mirroring the integer `status` column on books.
// Storage remains plain int in BookDTO/SQL — this type only exists so
// QML can write `BookStatus.Finished` instead of magic literal `3`.
class BookStatus {
  Q_GADGET
  QML_ELEMENT
  QML_UNCREATABLE("BookStatus is an enum namespace, not instantiable")

public:
  enum Value {
    None = 0,
    WantToRead = 1,
    InProgress = 2,
    Finished = 3,
  };
  Q_ENUM(Value)
};

} // namespace readary::services

#endif // READARY_SERVICES_BOOKSTATUS_HPP
