#ifndef READARY_CONTROLLERS_PROFILECONTROLLER_HPP
#define READARY_CONTROLLERS_PROFILECONTROLLER_HPP

#include "qmltypes/LibraryStatisticsObject.hpp"
#include "services/storage/BookTable.hpp"

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>

#include <cstdint>
#include <memory>

namespace readary::controllers {

// The profile page reads the library as a whole: how much of it is behind the
// reader, and what that adds up to. Everything it shows is recomputed from the
// database — the controller stores nothing of its own.
class ProfileController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(readary::qmltypes::LibraryStatisticsObject statistics READ statistics NOTIFY statisticsChanged FINAL)
  Q_PROPERTY(ReaderLevel readerLevel READ readerLevel NOTIFY statisticsChanged FINAL)
  Q_PROPERTY(bool hasData READ hasData NOTIFY statisticsChanged FINAL)

public:
  // The badge over the profile card. Derived from the finished-book count
  // alone, so it is a fact about the library rather than a stored setting —
  // QML owns the wording, this owns the thresholds.
  enum class ReaderLevel : std::uint8_t {
    Newcomer = 0, // nothing finished yet
    Reader,       // 1..4
    Bookworm,     // 5..19
    Bibliophile,  // 20 and up
  };
  Q_ENUM(ReaderLevel)

  explicit ProfileController(std::shared_ptr<services::BookTable> bookTable, QObject *parent);

  qmltypes::LibraryStatisticsObject statistics() const;
  ReaderLevel readerLevel() const;
  bool hasData() const;

  // Recomputes from the library as it stands now, and stays quiet when the
  // figures come back unchanged — the page is rebuilt on every open and asks
  // for this, and an emit there would re-render the whole page for an
  // identical result.
  Q_INVOKABLE void refresh();

  static ProfileController *create(QQmlEngine *engine, QJSEngine *scriptEngine);
  static void setInstance(ProfileController *instance);

signals:
  void statisticsChanged();

private:
  static constexpr int kReaderBooks = 1;
  static constexpr int kBookwormBooks = 5;
  static constexpr int kBibliophileBooks = 20;

  static ProfileController *s_instance;

  std::shared_ptr<services::BookTable> _bookTable;
  qmltypes::LibraryStatisticsObject _statistics;
};

} // namespace readary::controllers

#endif // READARY_CONTROLLERS_PROFILECONTROLLER_HPP
