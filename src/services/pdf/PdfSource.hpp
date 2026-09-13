#ifndef READARY_SERVICES_PDFSOURCE_HPP
#define READARY_SERVICES_PDFSOURCE_HPP

#include <QObject>
#include <QtQml/qqmlregistration.h>

namespace readary::services {

// QML-visible enum mirroring the integer `pdfSource` column on books.
// Storage remains plain int in BookDTO/SQL — this type only exists so QML can
// write `PdfSource.User` instead of magic literal `2`.
//
// The distinction carries a rule, not just provenance: a catalog-supplied PDF
// belongs to the catalog and cannot be replaced or removed, while one the user
// attached is theirs to swap or drop.
class PdfSource {
  Q_GADGET
  QML_ELEMENT
  QML_UNCREATABLE("PdfSource is an enum namespace, not instantiable")

public:
  enum Value {
    None = 0,
    Server = 1,
    User = 2,
  };
  Q_ENUM(Value)
};

} // namespace readary::services

#endif // READARY_SERVICES_PDFSOURCE_HPP
