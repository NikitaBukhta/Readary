#include "controllers/ProfileController.hpp"

#include "services/statistics/LibraryStatisticsCalculator.hpp"

#include <QLoggingCategory>
#include <QQmlEngine>

#include <utility>

namespace {
Q_LOGGING_CATEGORY(lcProfile, "readary.controllers.profile")
}

namespace readary::controllers {

ProfileController *ProfileController::s_instance = nullptr;

ProfileController::ProfileController(std::shared_ptr<services::BookTable> bookTable, QObject *parent)
    : QObject{parent}, _bookTable{std::move(bookTable)} {}

void ProfileController::setInstance(ProfileController *instance) { s_instance = instance; }

ProfileController *ProfileController::create(QQmlEngine *engine, QJSEngine *scriptEngine) {
  Q_UNUSED(engine)
  Q_UNUSED(scriptEngine)
  Q_ASSERT_X(s_instance, "ProfileController::create", "setInstance() must be called before the QML engine loads");
  QQmlEngine::setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
  return s_instance;
}

qmltypes::LibraryStatisticsObject ProfileController::statistics() const { return _statistics; }

ProfileController::ReaderLevel ProfileController::readerLevel() const {
  const int finished = _statistics.booksFinished;
  if (finished >= kBibliophileBooks) {
    return ReaderLevel::Bibliophile;
  }
  if (finished >= kBookwormBooks) {
    return ReaderLevel::Bookworm;
  }
  if (finished >= kReaderBooks) {
    return ReaderLevel::Reader;
  }
  return ReaderLevel::Newcomer;
}

bool ProfileController::hasData() const { return _statistics.booksTotal > 0; }

void ProfileController::refresh() {
  const services::LibraryStatisticsDTO computed =
      services::LibraryStatisticsCalculator::compute(_bookTable->getAllBooks(), _bookTable->getAllReadingSessions());
  if (computed == _statistics) {
    return;
  }

  _statistics = qmltypes::LibraryStatisticsObject{computed};
  qCInfo(lcProfile) << "Library statistics — books:" << _statistics.booksTotal
                    << "finished:" << _statistics.booksFinished << "pages:" << _statistics.pagesRead
                    << "seconds:" << _statistics.totalSeconds;
  emit statisticsChanged();
}

} // namespace readary::controllers
