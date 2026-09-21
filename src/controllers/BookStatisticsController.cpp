#include "controllers/BookStatisticsController.hpp"

#include "services/statistics/BookStatisticsCalculator.hpp"

#include <QDate>
#include <QLoggingCategory>
#include <QQmlEngine>

#include <utility>

namespace {
Q_LOGGING_CATEGORY(lcStatistics, "readary.controllers.statistics")
}

namespace readary::controllers {

BookStatisticsController *BookStatisticsController::s_instance = nullptr;

BookStatisticsController::BookStatisticsController(std::shared_ptr<services::BookTable> bookTable, QObject *parent)
    : QObject{parent}, _bookTable{std::move(bookTable)} {}

void BookStatisticsController::setInstance(BookStatisticsController *instance) { s_instance = instance; }

BookStatisticsController *BookStatisticsController::create(QQmlEngine *engine, QJSEngine *scriptEngine) {
  Q_UNUSED(engine)
  Q_UNUSED(scriptEngine)
  Q_ASSERT_X(s_instance, "BookStatisticsController::create",
             "setInstance() must be called before the QML engine loads");
  QQmlEngine::setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
  return s_instance;
}

qint64 BookStatisticsController::bookIsbn() const { return _bookIsbn; }

void BookStatisticsController::setBookIsbn(qint64 isbn) {
  // BookController re-emits currentBookIsbnChanged on every book-list reset,
  // not only when the open book changes, so without this guard an unrelated
  // edit — a wishlist toggle — would re-read the journal. Journal writes
  // arrive on readingJournalChanged; this setter only picks the book.
  if (_bookIsbn == isbn) {
    return;
  }
  _bookIsbn = isbn;
  refresh();
}

qmltypes::BookStatisticsObject BookStatisticsController::statistics() const { return _statistics; }

bool BookStatisticsController::hasData() const { return _statistics.sessionCount > 0; }

void BookStatisticsController::refresh() {
  const auto sessions =
      _bookIsbn > 0 ? _bookTable->getReadingSessions(_bookIsbn) : QList<services::ReadingSessionDTO>{};
  services::BookStatisticsDTO computed = services::BookStatisticsCalculator::compute(sessions, QDate::currentDate());
  if (computed == _statistics) {
    return;
  }

  _statistics = qmltypes::BookStatisticsObject{std::move(computed)};
  qCInfo(lcStatistics) << "Statistics for book isbn:" << _bookIsbn << "sessions:" << _statistics.sessionCount
                       << "pages:" << _statistics.pagesRead;
  emit statisticsChanged();
}

} // namespace readary::controllers
