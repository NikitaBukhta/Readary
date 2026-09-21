#ifndef READARY_CONTROLLERS_BOOKSTATISTICSCONTROLLER_HPP
#define READARY_CONTROLLERS_BOOKSTATISTICSCONTROLLER_HPP

#include "qmltypes/BookStatisticsObject.hpp"
#include "services/storage/BookTable.hpp"

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>

#include <memory>

namespace readary::controllers {

// The statistics page reads one book at a time. The ISBN is pushed in by
// AppInitializer whenever the open book changes, so the page itself only ever
// reads `statistics`.
class BookStatisticsController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(readary::qmltypes::BookStatisticsObject statistics READ statistics NOTIFY statisticsChanged FINAL)
  Q_PROPERTY(bool hasData READ hasData NOTIFY statisticsChanged FINAL)

public:
  explicit BookStatisticsController(std::shared_ptr<services::BookTable> bookTable, QObject *parent);

  // Not QML-visible: the page never picks a book, it only renders whichever
  // one AppInitializer pushed in.
  qint64 bookIsbn() const;
  void setBookIsbn(qint64 isbn);

  qmltypes::BookStatisticsObject statistics() const;
  bool hasData() const;

  // Recomputes from the journal as it stands now, and stays quiet when the
  // figures come back unchanged — the page is rebuilt on every open and asks
  // for this, and an emit there would tear down and rebuild every chart
  // delegate for an identical result.
  Q_INVOKABLE void refresh();

  static BookStatisticsController *create(QQmlEngine *engine, QJSEngine *scriptEngine);
  static void setInstance(BookStatisticsController *instance);

signals:
  void statisticsChanged();

private:
  static BookStatisticsController *s_instance;

  std::shared_ptr<services::BookTable> _bookTable;
  qint64 _bookIsbn{0};
  qmltypes::BookStatisticsObject _statistics;
};

} // namespace readary::controllers

#endif // READARY_CONTROLLERS_BOOKSTATISTICSCONTROLLER_HPP
