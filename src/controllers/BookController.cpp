#include "controllers/BookController.hpp"

#include "models/books/filters/BookFilterStrategy.hpp"
#include "models/books/list/BookListModel.hpp"
#include "models/books/proxy/BookSearchProxyModel.hpp"
#include "models/books/proxy/BookSortFilterProxyModel.hpp"
#include "services/caching/ReadingProgressCache.hpp"
#include "services/caching/ReadingSessionCache.hpp"
#include "services/dto/BookStatus.hpp"
#include "services/pdf/PdfMetadataReader.hpp"
#include "services/pdf/PdfSource.hpp"
#include "utils/IsbnValidator.hpp"

#include <QBuffer>
#include <QDesktopServices>
#include <QLoggingCategory>
#include <QUrl>

using Qt::StringLiterals::operator""_s;

namespace {
Q_LOGGING_CATEGORY(lcBook, "readary.controllers.book")

QString coverAsDataUrl(const QImage &cover) {
  if (cover.isNull()) {
    return {};
  }

  QByteArray png;
  QBuffer buffer{&png};
  if (!buffer.open(QIODevice::WriteOnly) || !cover.save(&buffer, "PNG")) {
    qCWarning(lcBook) << "Cannot encode the staged pdf cover";
    return {};
  }
  return u"data:image/png;base64,"_s + QString::fromLatin1(png.toBase64());
}
} // namespace

namespace readary::controllers {

BookController *BookController::s_instance = nullptr;

BookController::BookController(std::shared_ptr<services::BookTable> bookTable,
                               std::shared_ptr<services::BookFileStore> fileStore, models::BookListModel *listModel,
                               QObject *parent)
    : QObject{parent}, _bookTable{std::move(bookTable)}, _fileStore{std::move(fileStore)}, _listModel{listModel},
      _searchProxy{new models::BookSearchProxyModel{this}},
      _criteriaProxy{new models::BookCriteriaFilterProxyModel{this}},
      _charactersModel{new models::BookCharactersModel{_bookTable, this}},
      _readingHistoryModel{new models::ReadingHistoryModel{_bookTable, this}} {
  _listModel->refresh();

  _proxies.insert(ListKind::WantToRead, buildProxy(_listModel, models::filters::WantToReadFilterStrategy{}));
  _proxies.insert(ListKind::WantToBuy, buildProxy(_listModel, models::filters::WantToBuyFilterStrategy{}));
  _proxies.insert(ListKind::AlreadyRead, buildProxy(_listModel, models::filters::AlreadyReadFilterStrategy{}));
  _proxies.insert(ListKind::InProgress, buildProxy(_listModel, models::filters::ReadInProgressFilterStrategy{}));

  applyActiveSourceToSearchProxy();

  QObject::connect(this, &BookController::currentBookIsbnChanged, this, [this] {
    _charactersModel->setBookIsbn(_currentBookIsbn);
    _readingHistoryModel->setBookIsbn(_currentBookIsbn);
  });

  // bookSaved refreshes the list model (wired in AppInitializer) and that reset
  // lands here, which is what re-reads a session appended to the journal.
  QObject::connect(_listModel, &QAbstractItemModel::modelReset, this, [this] {
    if (!_cacheValid) {
      return;
    }
    _cacheValid = false;
    emit currentBookIsbnChanged();
  });
}

BookController::~BookController() = default;

void BookController::setInstance(BookController *instance) { s_instance = instance; }

BookController *BookController::create(QQmlEngine *engine, QJSEngine *scriptEngine) {
  Q_UNUSED(engine)
  Q_UNUSED(scriptEngine)
  Q_ASSERT_X(s_instance, "BookController::create", "setInstance() must be called before the QML engine loads");
  QQmlEngine::setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
  return s_instance;
}

qint64 BookController::currentBookIsbn() const { return _currentBookIsbn; }

void BookController::setCurrentBookIsbn(qint64 isbn) {
  if (_currentBookIsbn == isbn) {
    return;
  }
  _currentBookIsbn = isbn;
  _cacheValid = false;
  emit currentBookIsbnChanged();
}

qmltypes::BookDTOObject BookController::currentBookData() const {
  if (_currentBookIsbn <= 0) {
    return {};
  }

  if (_cacheValid) {
    return _cachedBookData;
  }

  auto base = _listModel->getBook(_currentBookIsbn);
  if (base.isbn == 0) {
    return {};
  }

  qmltypes::BookDTOObject result(std::move(base));
  result.genres = _bookTable->getGenres(_currentBookIsbn);
  _cachedBookData = std::move(result);
  _cacheValid = true;
  return _cachedBookData;
}

QString BookController::errorMessage() const { return _errorMessage; }

BookController::ListKind BookController::activeKind() const { return _activeKind; }

void BookController::setActiveKind(ListKind kind) {
  if (_activeKind == kind) {
    return;
  }
  _activeKind = kind;
  applyActiveSourceToSearchProxy();
  emit activeKindChanged();
}

models::BookSortFilterProxyModel *BookController::getSortFilterProxyForKind(ListKind kind) const {
  return _proxies.value(kind, nullptr);
}

void BookController::openBook(qint64 isbn) {
  if (isbn <= 0) {
    return;
  }
  setCurrentBookIsbn(isbn);
  emit bookOpenRequested(isbn);
}

void BookController::importAndOpenBook(const services::BookDTO &book) {
  if (book.isbn <= 0) {
    qCWarning(lcBook) << "Cannot import book without ISBN — name:" << book.name;
    return;
  }

  if (!_listModel->contains(book.isbn)) {
    if (_bookTable->addBook(book) == 0) {
      setErrorMessage(tr("Failed to import book."));
      return;
    }
    emit bookSaved();
  }

  openBook(book.isbn);
}

bool BookController::addCustomBook(const QVariantMap &fields) {
  services::BookDTO book = services::BookDTO::fromMap(fields);
  book.name = book.name.trimmed();
  book.authorName = book.authorName.trimmed();
  book.publisherName = book.publisherName.trimmed();
  book.description = book.description.trimmed();

  // The pdf is the harder evidence: whatever it says wins over the form. Only
  // for a hand-added book, which is the only kind whose fields the user typed.
  if (_stagedPdf.isValid()) {
    overrideFromPdf(book, _stagedPdf);
  }

  if (book.name.isEmpty()) {
    setErrorMessage(tr("Enter a title before adding the book."));
    return false;
  }

  if (book.authorName.isEmpty()) {
    setErrorMessage(tr("Enter an author before adding the book."));
    return false;
  }

  if (!resolveTypedIsbn(fields, book.isbn)) {
    return false;
  }
  if (_stagedPdf.isbn != 0) {
    book.isbn = _stagedPdf.isbn;
  }

  if (book.isbn > 0 && _listModel->contains(book.isbn)) {
    setErrorMessage(tr("That book is already in your library."));
    return false;
  }

  book.isCustom = true;
  book.status = services::BookStatus::WantToRead;

  // The row goes in first: with no ISBN its key comes from the table, and the
  // stored files are named after that key.
  const qint64 key = _bookTable->addBook(book);
  if (key <= 0) {
    setErrorMessage(tr("Failed to add the book."));
    return false;
  }
  book.isbn = key;

  // Files are named after the key, so they can only be written now. A file that
  // cannot be stored costs the book its picture, not its existence.
  const bool storedFiles = _stagedPdf.isValid() ? applyPdf(book, _stagedPdfPath, _stagedPdf) : adoptPickedCover(book);
  if (!storedFiles && _stagedPdf.isValid()) {
    qCWarning(lcBook) << "Book stored without its pdf — key:" << key;
  } else if (storedFiles && !_bookTable->updateBook(book)) {
    qCWarning(lcBook) << "Book stored, but its file paths were not — key:" << key;
  }

  clearStagedPdf();
  setErrorMessage({});
  qCInfo(lcBook) << "Added custom book key:" << key << "name:" << book.name;

  emit bookSaved();
  openBook(key);
  return true;
}

bool BookController::resolveTypedIsbn(const QVariantMap &fields, qint64 &isbn) {
  const QString typed = fields.value(u"isbnText"_s).toString().trimmed();
  if (typed.isEmpty()) {
    isbn = 0;
    return true;
  }

  const auto converted = utils::IsbnValidator::convert(typed);
  if (!converted) {
    // Refused rather than ignored: a mistyped ISBN quietly becoming "no ISBN"
    // would leave the user believing the book was filed under it.
    setErrorMessage(tr("That is not a valid ISBN."));
    return false;
  }

  isbn = *converted;
  return true;
}

bool BookController::deleteCurrentBook() {
  if (_currentBookIsbn <= 0) {
    return false;
  }

  if (!currentBookData().isCustom) {
    // A catalog book leaves a category, never the library — it can always be
    // found again through the online search.
    qCWarning(lcBook) << "Refusing to delete a book that was not added by hand — isbn:" << _currentBookIsbn;
    setErrorMessage(tr("Only books you added yourself can be deleted."));
    return false;
  }

  const qint64 isbn = _currentBookIsbn;

  // Characters, genres and reading sessions go with it through ON DELETE CASCADE.
  if (!_bookTable->deleteBook(isbn)) {
    setErrorMessage(tr("Failed to delete the book."));
    return false;
  }

  // Only after the row is gone: files with no row are garbage, a row with no
  // files would be a broken book. The caches follow for the same reason — a
  // failed delete must not cost the user their parked progress.
  _fileStore->removeAll(isbn);
  services::ReadingProgressCache::clear(isbn);
  services::ReadingSessionCache::clear(isbn);

  // Cleared before the refresh so currentBookData() stops resolving a row that
  // no longer exists.
  setCurrentBookIsbn(0);
  setErrorMessage({});
  qCInfo(lcBook) << "Deleted custom book isbn:" << isbn;
  emit bookSaved();
  return true;
}

QVariantMap BookController::stagePdf(const QString &fileUrl) {
  const QString path = services::BookFileStore::toLocalPath(fileUrl);
  const services::PdfDocumentInfo info = services::PdfMetadataReader::read(path);
  if (!info.isValid()) {
    clearStagedPdf();
    setErrorMessage(tr("That file could not be read as a PDF."));
    return {{u"ok"_s, false}, {u"error"_s, _errorMessage}};
  }

  _stagedPdfPath = path;
  _stagedPdf = info;
  setErrorMessage({});

  return {
      {u"ok"_s, true},
      {u"pageCount"_s, info.pageCount},
      {u"title"_s, info.title},
      {u"author"_s, info.author},
      {u"coverPreview"_s, coverAsDataUrl(info.cover)},
      // A string: a 13-digit ISBN is exact as a JS number, but every other ISBN
      // crossing this boundary travels as text and the field shows it verbatim.
      {u"isbn"_s, info.isbn > 0 ? QString::number(info.isbn) : QString{}},
  };
}

void BookController::clearStagedPdf() {
  _stagedPdfPath.clear();
  _stagedPdf = {};
}

bool BookController::attachPdfToCurrentBook(const QString &fileUrl) {
  if (_currentBookIsbn <= 0) {
    return false;
  }

  qmltypes::BookDTOObject book = currentBookData();
  if (book.pdfSource == services::PdfSource::Server) {
    qCWarning(lcBook) << "Refusing to replace a catalog-supplied pdf — isbn:" << _currentBookIsbn;
    setErrorMessage(tr("This PDF came with the book and cannot be replaced."));
    return false;
  }

  const QString path = services::BookFileStore::toLocalPath(fileUrl);
  const services::PdfDocumentInfo info = services::PdfMetadataReader::read(path);
  if (!info.isValid()) {
    setErrorMessage(tr("That file could not be read as a PDF."));
    return false;
  }

  if (!applyPdf(book, path, info)) {
    setErrorMessage(tr("Failed to store the PDF."));
    return false;
  }

  if (!_bookTable->updateBook(book)) {
    setErrorMessage(tr("Failed to attach the PDF."));
    return false;
  }

  setErrorMessage({});
  qCInfo(lcBook) << "Attached pdf — book isbn:" << _currentBookIsbn << "pages:" << info.pageCount;
  emit bookSaved();
  return true;
}

bool BookController::removePdfFromCurrentBook() {
  if (_currentBookIsbn <= 0) {
    return false;
  }

  qmltypes::BookDTOObject book = currentBookData();
  if (book.pdfSource != services::PdfSource::User) {
    qCWarning(lcBook) << "Refusing to remove a pdf the user did not attach — isbn:" << _currentBookIsbn;
    setErrorMessage(tr("Only a PDF you attached yourself can be removed."));
    return false;
  }

  // The row first: a failed update after the file was gone would leave the book
  // offering to open a pdf that no longer exists. The cover stays either way —
  // it is the book's picture now, not part of the pdf.
  book.pdfPath.clear();
  book.pdfSource = services::PdfSource::None;
  if (!_bookTable->updateBook(book)) {
    setErrorMessage(tr("Failed to remove the PDF."));
    return false;
  }

  if (!_fileStore->removePdf(_currentBookIsbn)) {
    qCWarning(lcBook) << "Pdf detached from the book but its file remains — isbn:" << _currentBookIsbn;
  }

  setErrorMessage({});
  qCInfo(lcBook) << "Removed pdf — book isbn:" << _currentBookIsbn;
  emit bookSaved();
  return true;
}

bool BookController::openCurrentBookPdf() const {
  const QString path = currentBookData().pdfPath;
  if (path.isEmpty()) {
    return false;
  }
  // Handed to the platform viewer: rendering a whole book is not this app's job.
  return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

bool BookController::adoptPickedCover(services::BookDTO &book) const {
  const QUrl picked{book.coverUrl};
  if (book.coverUrl.isEmpty() || (!picked.isLocalFile() && !picked.isRelative() && picked.scheme() != u"content"_s)) {
    // A catalog cover is a remote url and stays one; there is nothing to copy.
    return false;
  }

  const QImage cover{services::BookFileStore::toLocalPath(book.coverUrl)};
  if (cover.isNull()) {
    qCWarning(lcBook) << "Cannot read the picked cover:" << book.coverUrl;
    return false;
  }

  // Same bound the pdf render uses — a cover picked from a phone camera would
  // otherwise be stored at full resolution.
  const QImage scaled =
      cover.size().boundedTo(services::PdfMetadataReader::kDefaultCoverSize) == cover.size()
          ? cover
          : cover.scaled(services::PdfMetadataReader::kDefaultCoverSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  if (_fileStore->storeCover(book.isbn, scaled).isEmpty()) {
    return false;
  }

  // Copied in rather than referenced: the picked file may be moved or deleted,
  // and on Android its content:// grant does not survive a restart.
  book.coverUrl = _fileStore->coverUrl(book.isbn);
  return true;
}

void BookController::overrideFromPdf(services::BookDTO &book, const services::PdfDocumentInfo &info) {
  book.totalPages = info.pageCount;

  // Blanks do not override: plenty of PDFs carry an empty /Info dictionary, and
  // wiping what the user typed with nothing would be a loss, not a correction.
  if (!info.title.isEmpty()) {
    book.name = info.title;
  }
  if (!info.author.isEmpty()) {
    book.authorName = info.author;
  }
  if (!info.subject.isEmpty()) {
    book.description = info.subject;
  }
}

bool BookController::applyPdf(services::BookDTO &book, const QString &sourcePath,
                              const services::PdfDocumentInfo &info) const {
  const QString storedPdf = _fileStore->storePdf(book.isbn, sourcePath);
  if (storedPdf.isEmpty()) {
    return false;
  }

  book.pdfPath = storedPdf;
  book.pdfSource = services::PdfSource::User;

  if (book.isCustom) {
    overrideFromPdf(book, info);
  }

  // A rendered first page beats no cover, and for a custom book it beats the
  // one the user picked by hand — same rule as the rest of the metadata.
  // Written only when it will actually be adopted: a catalog book that already
  // has a cover would otherwise leave a file nothing ever references, and one
  // that is never deleted since catalog books cannot be.
  if ((book.isCustom || book.coverUrl.isEmpty()) && !_fileStore->storeCover(book.isbn, info.cover).isEmpty()) {
    book.coverUrl = _fileStore->coverUrl(book.isbn);
  }

  return true;
}

void BookController::saveReadingSession(const QString &bookIsbn, int seconds, int phase) {
  const qint64 isbn = bookIsbn.toLongLong();
  if (isbn <= 0) {
    return;
  }
  services::ReadingSessionCache::save(isbn, seconds, phase);
}

QVariantMap BookController::takeReadingSession(const QString &bookIsbn) {
  const qint64 isbn = bookIsbn.toLongLong();
  if (isbn <= 0) {
    return {};
  }
  return services::ReadingSessionCache::takeState(isbn);
}

void BookController::clearReadingSession(const QString &bookIsbn) {
  const qint64 isbn = bookIsbn.toLongLong();
  if (isbn <= 0) {
    return;
  }
  services::ReadingSessionCache::clear(isbn);
}

void BookController::setBookStatus(int status) {
  if (_currentBookIsbn <= 0) {
    return;
  }

  qmltypes::BookDTOObject book = currentBookData();
  if (book.status == status) {
    return;
  }

  book.status = status;
  if (!_bookTable->updateBook(book)) {
    setErrorMessage(tr("Failed to update book status."));
    return;
  }

  qCInfo(lcBook) << "Set status — book isbn:" << _currentBookIsbn << "status:" << status;
  emit bookSaved();
}

void BookController::toggleWantToRead() {
  if (_currentBookIsbn <= 0) {
    return;
  }

  const int current = currentBookData().status;
  setBookStatus(current == services::BookStatus::WantToRead ? services::BookStatus::None
                                                            : services::BookStatus::WantToRead);
}

void BookController::toggleWishList() {
  if (_currentBookIsbn <= 0) {
    return;
  }

  qmltypes::BookDTOObject book = currentBookData();
  book.inWishList = !book.inWishList;
  if (!_bookTable->updateBook(book)) {
    setErrorMessage(tr("Failed to update wishlist."));
    return;
  }

  qCInfo(lcBook) << "Toggled wishlist — book isbn:" << _currentBookIsbn << "inWishList:" << book.inWishList;
  emit bookSaved();
}

void BookController::moveInProgressToWantToRead() {
  if (_currentBookIsbn <= 0) {
    return;
  }

  qmltypes::BookDTOObject book = currentBookData();
  if (book.status != services::BookStatus::InProgress) {
    return;
  }

  if (book.pagesRead > 0) {
    services::ReadingProgressCache::save(_currentBookIsbn, book.pagesRead);
  }
  services::ReadingSessionCache::clear(_currentBookIsbn);

  book.status = services::BookStatus::WantToRead;
  book.pagesRead = 0;
  if (!_bookTable->updateBook(book)) {
    setErrorMessage(tr("Failed to update book status."));
    return;
  }

  qCInfo(lcBook) << "Moved in-progress book to want-to-read — book isbn:" << _currentBookIsbn;
  emit bookSaved();
}

bool BookController::hasCachedProgress() const { return services::ReadingProgressCache::has(_currentBookIsbn); }

void BookController::restoreCachedProgress() {
  if (_currentBookIsbn <= 0 || !services::ReadingProgressCache::has(_currentBookIsbn)) {
    return;
  }

  qmltypes::BookDTOObject book = currentBookData();
  book.pagesRead = services::ReadingProgressCache::takePagesRead(_currentBookIsbn);
  book.status = services::BookStatus::InProgress;
  if (!_bookTable->updateBook(book)) {
    setErrorMessage(tr("Failed to restore reading progress."));
    return;
  }

  qCInfo(lcBook) << "Restored cached progress — book isbn:" << _currentBookIsbn << "pagesRead:" << book.pagesRead;
  emit bookSaved();
}

void BookController::discardCachedProgress() const { services::ReadingProgressCache::clear(_currentBookIsbn); }

void BookController::updateReadingProgress(int pageNumber, int durationSeconds) {
  if (_currentBookIsbn <= 0) {
    return;
  }

  qmltypes::BookDTOObject book = currentBookData();
  const int pagesFrom = book.pagesRead;
  if (book.pagesRead >= pageNumber || pageNumber > book.totalPages) {
    qCDebug(lcBook) << "Not updating reading progress — invalid page number:" << pageNumber
                    << "current pages read:" << book.pagesRead << "total pages:" << book.totalPages;
    return;
  }
  book.status = pageNumber == book.totalPages ? services::BookStatus::Finished : services::BookStatus::InProgress;
  book.pagesRead = pageNumber;

  if (!_bookTable->updateBook(book)) {
    setErrorMessage(tr("Failed to update reading progress."));
    return;
  }

  if (durationSeconds > 0) {
    if (_bookTable->insertReadingSession(_currentBookIsbn, pagesFrom, pageNumber, durationSeconds) > 0) {
      emit readingJournalChanged();
    } else {
      qCWarning(lcBook) << "Reading progress saved, but session log insert failed for book isbn:" << _currentBookIsbn;
    }
  }

  emit bookSaved();
}

void BookController::deleteReadingSession(const QString &sessionId) {
  // A string rather than qint64: 64-bit values lose their magnitude crossing the
  // QML boundary, as with the ISBN the timer invokables above take.
  bool parsed = false;
  const qint64 id = sessionId.toLongLong(&parsed);
  if (!parsed || id <= 0) {
    qCWarning(lcBook) << "Ignoring delete of reading session with invalid id:" << sessionId;
    return;
  }

  if (!_bookTable->deleteReadingSession(id)) {
    setErrorMessage(tr("Failed to delete the reading session."));
    return;
  }

  // Same ISBN: keeps however far the list was expanded.
  _readingHistoryModel->setBookIsbn(_currentBookIsbn);
  emit readingJournalChanged();
}

models::BookSearchProxyModel *BookController::searchModel() const { return _searchProxy; }

models::BookCharactersModel *BookController::charactersModel() const { return _charactersModel; }

models::ReadingHistoryModel *BookController::readingHistoryModel() const { return _readingHistoryModel; }

models::BookSortFilterProxyModel *BookController::buildProxy(models::BookListModel *source,
                                                             const models::filters::BookFilterStrategy &strategy) {
  auto *proxy = new models::BookSortFilterProxyModel{this};
  proxy->setSourceModel(source);
  strategy.apply(proxy);
  return proxy;
}

void BookController::applyActiveSourceToSearchProxy() {
  if (_searchProxy == nullptr) {
    return;
  }

  _criteriaProxy->setSourceModel(getSortFilterProxyForKind(_activeKind));
  _searchProxy->setSourceModel(_criteriaProxy);
}

void BookController::setFilterCriteria(const services::BookFilterCriteria &criteria) {
  _criteriaProxy->setCriteria(criteria);
}

void BookController::setErrorMessage(const QString &message) {
  if (_errorMessage == message) {
    return;
  }
  _errorMessage = message;
  emit errorMessageChanged();
}

} // namespace readary::controllers
