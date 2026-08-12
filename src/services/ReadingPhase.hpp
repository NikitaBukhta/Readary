#ifndef READARY_SERVICES_READINGPHASE_HPP
#define READARY_SERVICES_READINGPHASE_HPP

#include <QObject>
#include <QtQml/qqmlregistration.h>

namespace readary::services {

// Reading-timer phase shared between C++ (ReadingSessionCache) and QML
// (ReadingProgressTimer). A single source of truth keeps the integer values
// in sync across the language boundary.
class ReadingPhase {
  Q_GADGET
  QML_ELEMENT
  QML_UNCREATABLE("ReadingPhase is an enum namespace, not instantiable")

public:
  enum Value {
    Stopped = 0,
    Running = 1,
    Paused = 2,
  };
  Q_ENUM(Value)
};

} // namespace readary::services

#endif // READARY_SERVICES_READINGPHASE_HPP
