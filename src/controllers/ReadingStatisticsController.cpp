#include "controllers/ReadingStatisticsController.hpp"

#include "services/dto/BookStatus.hpp"
#include "services/statistics/ReadingStatisticsCalculator.hpp"
#include "services/statistics/StatisticsPeriods.hpp"

#include <QLoggingCategory>
#include <QQmlEngine>

#include <algorithm>
#include <utility>

namespace {
Q_LOGGING_CATEGORY(lcReadingStatistics, "readary.controllers.readingStatistics")
}

namespace readary::controllers {

static_assert(static_cast<int>(ReadingStatisticsController::Granularity::ByYear) + 1 ==
                  static_cast<int>(services::StatisticsGranularity::Count),
              "ReadingStatisticsController::Granularity must list every services::StatisticsGranularity");

ReadingStatisticsController *ReadingStatisticsController::s_instance = nullptr;

ReadingStatisticsController::ReadingStatisticsController(std::shared_ptr<services::BookTable> bookTable,
                                                         QObject *parent)
    : ReadingStatisticsController{std::move(bookTable), &QDate::currentDate, parent} {}

ReadingStatisticsController::ReadingStatisticsController(std::shared_ptr<services::BookTable> bookTable, Clock today,
                                                         QObject *parent)
    : QObject{parent}, _bookTable{std::move(bookTable)}, _today{std::move(today)} {}

void ReadingStatisticsController::setInstance(ReadingStatisticsController *instance) { s_instance = instance; }

ReadingStatisticsController *ReadingStatisticsController::create(QQmlEngine *engine, QJSEngine *scriptEngine) {
  Q_UNUSED(engine)
  Q_UNUSED(scriptEngine)
  Q_ASSERT_X(s_instance, "ReadingStatisticsController::create",
             "setInstance() must be called before the QML engine loads");
  QQmlEngine::setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
  return s_instance;
}

const qmltypes::ReadingStatisticsObject &ReadingStatisticsController::statistics() const { return _statistics; }

bool ReadingStatisticsController::hasData() const { return _hasData; }

ReadingStatisticsController::Period ReadingStatisticsController::period() const { return _period; }

void ReadingStatisticsController::setPeriod(Period period) {
  if (!isKnown(period)) {
    qCWarning(lcReadingStatistics) << "Ignoring unknown statistics period:" << static_cast<int>(period);
    return;
  }
  if (_period == period) {
    return;
  }
  _period = period;
  emit periodChanged();
  recompute();
}

bool ReadingStatisticsController::setCustomRange(const QString &from, const QString &to) {
  const QDate fromDate = QDate::fromString(from, Qt::ISODate);
  const QDate toDate = QDate::fromString(to, Qt::ISODate);
  if (!fromDate.isValid() || !toDate.isValid()) {
    qCWarning(lcReadingStatistics) << "Ignoring custom range that does not parse:" << from << to;
    return false;
  }

  _customFrom = fromDate;
  _customTo = toDate;
  if (_period != Period::Custom) {
    _period = Period::Custom;
    emit periodChanged();
  }
  recompute();
  return true;
}

void ReadingStatisticsController::refresh() {
  _books = _bookTable->getAllBooks();
  _sessions = _bookTable->getAllReadingSessions();
  _loaded = true;

  const bool hasData = !_sessions.isEmpty() || std::ranges::any_of(_books, [](const services::BookDTO &book) {
    return book.status == services::BookStatus::Finished || book.status == services::BookStatus::InProgress;
  });
  const bool hasDataChanged = hasData != _hasData;
  _hasData = hasData;
  recompute(hasDataChanged);
}

bool ReadingStatisticsController::isKnown(Period period) {
  switch (period) {
  case Period::Day:
  case Period::Week:
  case Period::Month:
  case Period::Year:
  case Period::AllTime:
  case Period::Custom:
    return true;
  }
  return false;
}

services::StatisticsRange ReadingStatisticsController::currentRange() const {
  const QDate today = _today();
  switch (_period) {
  case Period::Day:
    return services::StatisticsPeriods::day(today);
  case Period::Week:
    return services::StatisticsPeriods::week(today);
  case Period::Month:
    return services::StatisticsPeriods::month(today);
  case Period::Year:
    return services::StatisticsPeriods::year(today);
  case Period::Custom:
    return _customFrom.isValid() ? services::StatisticsPeriods::custom(_customFrom, _customTo)
                                 : services::StatisticsPeriods::day(today);
  case Period::AllTime:
    break;
  }
  return services::StatisticsPeriods::allTime(today, services::ReadingStatisticsCalculator::earliestSession(_sessions));
}

void ReadingStatisticsController::recompute(bool hasDataChanged) {
  if (!_loaded) {
    refresh();
    return;
  }

  const services::StatisticsRange range = currentRange();
  services::ReadingStatisticsDTO computed = services::ReadingStatisticsCalculator::compute(_books, _sessions, range);
  if (computed == _statistics && !hasDataChanged) {
    return;
  }

  _statistics = qmltypes::ReadingStatisticsObject{std::move(computed)};
  qCInfo(lcReadingStatistics) << "Reading statistics — period:" << _period << "range:" << range.from << range.to
                              << "finished:" << _statistics.booksFinished << "sessions:" << _statistics.sessionCount;
  emit statisticsChanged();
}

} // namespace readary::controllers
