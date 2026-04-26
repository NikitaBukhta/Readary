#include "BookFormController.hpp"
#include "models/BookListModel.hpp"

#include <QDate>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcBookForm, "bl.controllers.bookform")

namespace bl::controllers {

static const QString kIsbnPrefix = QStringLiteral("ISBN-");

BookFormController *BookFormController::s_instance = nullptr;

BookFormController::BookFormController(
    std::shared_ptr<services::BookTable> bookTable,
    bl::models::BookListModel *listModel, QObject *parent)
    : QObject(parent), _bookTable{std::move(bookTable)}, _listModel{listModel} {
}

void BookFormController::setInstance(BookFormController *instance) {
  s_instance = instance;
}

BookFormController *BookFormController::create(QQmlEngine *, QJSEngine *) {
  Q_ASSERT_X(s_instance, "BookFormController::create",
             "setInstance() must be called before the QML engine loads");
  QQmlEngine::setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
  return s_instance;
}

int BookFormController::currentBookId() const { return _currentBookId; }

void BookFormController::setCurrentBookId(int id) {
  if (_currentBookId == id)
    return;
  _currentBookId = id;
  emit currentBookIdChanged();
}

QVariantMap BookFormController::currentBookData() const {
  if (_currentBookId <= 0)
    return {};

  auto book = _listModel->getBook(_currentBookId);
  if (!book.isEmpty())
    book.insert("rawIsbn", stripIsbnPrefix(book.value("isbn").toString()));
  return book;
}

bool BookFormController::editMode() const { return _currentBookId > 0; }

QString BookFormController::errorMessage() const { return _errorMessage; }

int BookFormController::yearMin() const { return 1; }

int BookFormController::yearMax() const { return QDate::currentDate().year(); }

QString BookFormController::normalizeIsbn(const QString &rawIsbn) {
  QString trimmed = rawIsbn.trimmed();
  if (trimmed.isEmpty())
    return {};
  if (trimmed.startsWith(kIsbnPrefix))
    return trimmed;
  return kIsbnPrefix + trimmed;
}

QString BookFormController::stripIsbnPrefix(const QString &isbn) {
  if (isbn.startsWith(kIsbnPrefix))
    return isbn.mid(kIsbnPrefix.length());
  return isbn;
}

bool BookFormController::validate(const QString &title, const QString &author,
                                  int year, const QString &isbn) {
  if (title.trimmed().isEmpty()) {
    setErrorMessage(tr("Title is required."));
    return false;
  }

  if (author.trimmed().isEmpty()) {
    setErrorMessage(tr("Author is required."));
    return false;
  }

  if (year != 0 && (year < yearMin() || year > yearMax())) {
    setErrorMessage(
        tr("Year must be between %1 and %2.").arg(yearMin()).arg(yearMax()));
    return false;
  }

  return true;
}

void BookFormController::setErrorMessage(const QString &message) {
  if (_errorMessage == message)
    return;
  _errorMessage = message;
  emit errorMessageChanged();
}

} // namespace bl::controllers
