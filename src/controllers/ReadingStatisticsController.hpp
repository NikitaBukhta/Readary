#ifndef READARY_CONTROLLERS_READINGSTATISTICSCONTROLLER_HPP
#define READARY_CONTROLLERS_READINGSTATISTICSCONTROLLER_HPP

#include "qmltypes/ReadingStatisticsObject.hpp"
#include "services/storage/BookTable.hpp"

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>

#include <memory>

namespace readary::controllers {

// The reading-statistics page: BookStatisticsController's charts, summed over
// the whole library. Everything it shows is recomputed from the database — the
// controller stores nothing of its own.
class ReadingStatisticsController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(readary::qmltypes::ReadingStatisticsObject statistics READ statistics NOTIFY statisticsChanged FINAL)
  Q_PROPERTY(bool hasData READ hasData NOTIFY statisticsChanged FINAL)

public:
  explicit ReadingStatisticsController(std::shared_ptr<services::BookTable> bookTable, QObject *parent);

  qmltypes::ReadingStatisticsObject statistics() const;
  bool hasData() const;

  // Recomputes from the library as it stands now, and stays quiet when the
  // figures come back unchanged — the page is rebuilt on every open and asks
  // for this, and an emit there would tear down and rebuild every chart
  // delegate for an identical result.
  Q_INVOKABLE void refresh();

  static ReadingStatisticsController *create(QQmlEngine *engine, QJSEngine *scriptEngine);
  static void setInstance(ReadingStatisticsController *instance);

signals:
  void statisticsChanged();

private:
  static ReadingStatisticsController *s_instance;

  std::shared_ptr<services::BookTable> _bookTable;
  qmltypes::ReadingStatisticsObject _statistics;
};

} // namespace readary::controllers

#endif // READARY_CONTROLLERS_READINGSTATISTICSCONTROLLER_HPP
