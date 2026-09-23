#include "controllers/ReadingStatisticsController.hpp"

#include "services/statistics/ReadingStatisticsCalculator.hpp"

#include <QDate>
#include <QLoggingCategory>
#include <QQmlEngine>

#include <utility>

namespace {
Q_LOGGING_CATEGORY(lcReadingStatistics, "readary.controllers.readingStatistics")
}

namespace readary::controllers {

ReadingStatisticsController *ReadingStatisticsController::s_instance = nullptr;

ReadingStatisticsController::ReadingStatisticsController(std::shared_ptr<services::BookTable> bookTable,
                                                         QObject *parent)
    : QObject{parent}, _bookTable{std::move(bookTable)} {}

void ReadingStatisticsController::setInstance(ReadingStatisticsController *instance) { s_instance = instance; }

ReadingStatisticsController *ReadingStatisticsController::create(QQmlEngine *engine, QJSEngine *scriptEngine) {
  Q_UNUSED(engine)
  Q_UNUSED(scriptEngine)
  Q_ASSERT_X(s_instance, "ReadingStatisticsController::create",
             "setInstance() must be called before the QML engine loads");
  QQmlEngine::setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
  return s_instance;
}

qmltypes::ReadingStatisticsObject ReadingStatisticsController::statistics() const { return _statistics; }

bool ReadingStatisticsController::hasData() const {
  return _statistics.sessionCount > 0 || _statistics.booksFinished > 0 || !_statistics.booksInProgress.isEmpty();
}

void ReadingStatisticsController::refresh() {
  services::ReadingStatisticsDTO computed = services::ReadingStatisticsCalculator::compute(
      _bookTable->getAllBooks(), _bookTable->getAllReadingSessions(), QDate::currentDate());
  if (computed == _statistics) {
    return;
  }

  _statistics = qmltypes::ReadingStatisticsObject{std::move(computed)};
  qCInfo(lcReadingStatistics) << "Reading statistics — finished:" << _statistics.booksFinished
                              << "sessions:" << _statistics.sessionCount << "seconds:" << _statistics.totalSeconds;
  emit statisticsChanged();
}

} // namespace readary::controllers
