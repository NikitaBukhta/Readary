#ifndef READARY_CONTROLLERS_READINGSTATISTICSCONTROLLER_HPP
#define READARY_CONTROLLERS_READINGSTATISTICSCONTROLLER_HPP

#include "qmltypes/ReadingStatisticsObject.hpp"
#include "services/dto/BookDTO.hpp"
#include "services/dto/ReadingSessionDTO.hpp"
#include "services/dto/StatisticsRange.hpp"
#include "services/storage/BookTable.hpp"

#include <QDate>
#include <QJSEngine>
#include <QList>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QtQml/qqmlregistration.h>

#include <cstdint>
#include <functional>
#include <memory>

namespace readary::controllers {

class ReadingStatisticsController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(readary::qmltypes::ReadingStatisticsObject statistics READ statistics NOTIFY statisticsChanged FINAL)
  Q_PROPERTY(bool hasData READ hasData NOTIFY statisticsChanged FINAL)
  Q_PROPERTY(Period period READ period WRITE setPeriod NOTIFY periodChanged FINAL)

public:
  enum class Period : std::uint8_t {
    Day,
    Week,
    Month,
    Year,
    AllTime,
    Custom,
  };
  Q_ENUM(Period)

  enum class Granularity : std::uint8_t {
    ByHour = static_cast<std::uint8_t>(services::StatisticsGranularity::Hour),
    ByDay = static_cast<std::uint8_t>(services::StatisticsGranularity::Day),
    ByMonth = static_cast<std::uint8_t>(services::StatisticsGranularity::Month),
    ByYear = static_cast<std::uint8_t>(services::StatisticsGranularity::Year),
  };
  Q_ENUM(Granularity)

  using Clock = std::function<QDate()>;

  explicit ReadingStatisticsController(std::shared_ptr<services::BookTable> bookTable, QObject *parent);
  ReadingStatisticsController(std::shared_ptr<services::BookTable> bookTable, Clock today, QObject *parent);

  const qmltypes::ReadingStatisticsObject &statistics() const;
  bool hasData() const;

  Period period() const;
  void setPeriod(Period period);

  Q_INVOKABLE bool setCustomRange(const QString &from, const QString &to);

  Q_INVOKABLE void refresh();

  static ReadingStatisticsController *create(QQmlEngine *engine, QJSEngine *scriptEngine);
  static void setInstance(ReadingStatisticsController *instance);

signals:
  void statisticsChanged();
  void periodChanged();

private:
  static bool isKnown(Period period);
  services::StatisticsRange currentRange() const;
  void recompute(bool hasDataChanged = false);

  static ReadingStatisticsController *s_instance;

  std::shared_ptr<services::BookTable> _bookTable;
  Clock _today;
  QList<services::BookDTO> _books;
  QList<services::ReadingSessionDTO> _sessions;
  bool _loaded{false};
  qmltypes::ReadingStatisticsObject _statistics;
  bool _hasData{false};
  Period _period{Period::AllTime};
  QDate _customFrom;
  QDate _customTo;
};

} // namespace readary::controllers

#endif // READARY_CONTROLLERS_READINGSTATISTICSCONTROLLER_HPP
