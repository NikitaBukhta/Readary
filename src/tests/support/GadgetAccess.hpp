#ifndef READARY_TESTS_SUPPORT_GADGETACCESS_HPP
#define READARY_TESTS_SUPPORT_GADGETACCESS_HPP

#include <QMetaObject>
#include <QMetaProperty>
#include <QVariant>

namespace readary::tests {

// Reads and writes a Q_GADGET property through the metaobject, the way QML
// does. A gadget has no QObject to go through, so every gadget test needs
// this pair; an absent property comes back invalid rather than asserting, so
// a test can pin that a name is *not* exposed.
template <typename Gadget> QVariant readGadget(const Gadget &gadget, const char *property) {
  const QMetaObject &meta = Gadget::staticMetaObject;
  const int index = meta.indexOfProperty(property);
  if (index < 0) {
    return {};
  }
  return meta.property(index).readOnGadget(&gadget);
}

template <typename Gadget> bool writeGadget(Gadget &gadget, const char *property, const QVariant &value) {
  const QMetaObject &meta = Gadget::staticMetaObject;
  const int index = meta.indexOfProperty(property);
  if (index < 0) {
    return false;
  }
  return meta.property(index).writeOnGadget(&gadget, value);
}

} // namespace readary::tests

#endif // READARY_TESTS_SUPPORT_GADGETACCESS_HPP
