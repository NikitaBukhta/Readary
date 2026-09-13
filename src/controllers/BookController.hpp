#ifndef READARY_CONTROLLERS_BOOKCONTROLLER_HPP
#define READARY_CONTROLLERS_BOOKCONTROLLER_HPP

#include "models/books/BookCharactersModel.hpp"
#include "models/books/BookCriteriaFilterProxyModel.hpp"
#include "models/books/BookSearchProxyModel.hpp"
#include "models/books/BookSortFilterProxyModel.hpp"
#include "models/books/ReadingHistoryModel.hpp"
#include "qmltypes/BookDTOObject.hpp"
#include "services/BookFileStore.hpp"
#include "services/BookTable.hpp"
#include "services/PdfDocumentInfo.hpp"

#include <QHash>
#include <QObject>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>

#include <cstdint>
#include <memory>

namespace readary::models {
class BookListModel;
namespace filters {
class BookFilterStrategy;
}
} // namespace readary::models

namespace readary::controllers {

class BookController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(qint64 currentBookIsbn READ currentBookIsbn WRITE setCurrentBookIsbn NOTIFY currentBookIsbnChanged)
  Q_PROPERTY(readary::qmltypes::BookDTOObject currentBookData READ currentBookData NOTIFY currentBookIsbnChanged)
  Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

  Q_PROPERTY(ListKind activeKind READ activeKind WRITE setActiveKind NOTIFY activeKindChanged)
  Q_PROPERTY(readary::models::BookSearchProxyModel *searchModel READ searchModel CONSTANT)
  Q_PROPERTY(readary::models::BookCharactersModel *charactersModel READ charactersModel CONSTANT)
  Q_PROPERTY(readary::models::ReadingHistoryModel *readingHistoryModel READ readingHistoryModel CONSTANT)

public:
  enum class ListKind : uint8_t {
    WantToRead = 0,
    WantToBuy = 1,
    AlreadyRead = 2,
    InProgress = 3,
  };
  Q_ENUM(ListKind)

  explicit BookController(std::shared_ptr<services::BookTable> bookTable,
                          std::shared_ptr<services::BookFileStore> fileStore, models::BookListModel *listModel,
                          QObject *parent);
  ~BookController() override;

  qint64 currentBookIsbn() const;
  void setCurrentBookIsbn(qint64 isbn);
  qmltypes::BookDTOObject currentBookData() const;
  QString errorMessage() const;

  ListKind activeKind() const;
  void setActiveKind(ListKind kind);
  Q_INVOKABLE models::BookSortFilterProxyModel *getSortFilterProxyForKind(ListKind kind) const;

  void setFilterCriteria(const services::BookFilterCriteria &criteria);

  Q_INVOKABLE void openBook(qint64 isbn);
  void importAndOpenBook(const services::BookDTO &book);
  Q_INVOKABLE bool addCustomBook(const QVariantMap &fields);
  Q_INVOKABLE bool deleteCurrentBook();

  Q_INVOKABLE QVariantMap stagePdf(const QString &fileUrl);
  Q_INVOKABLE void clearStagedPdf();
  Q_INVOKABLE bool attachPdfToCurrentBook(const QString &fileUrl);
  Q_INVOKABLE bool removePdfFromCurrentBook();
  Q_INVOKABLE bool openCurrentBookPdf() const;

  static Q_INVOKABLE void saveReadingSession(const QString &bookIsbn, int seconds, int phase);
  static Q_INVOKABLE QVariantMap takeReadingSession(const QString &bookIsbn);
  static Q_INVOKABLE void clearReadingSession(const QString &bookIsbn);
  Q_INVOKABLE void setBookStatus(int status);
  Q_INVOKABLE void toggleWantToRead();
  Q_INVOKABLE void toggleWishList();
  Q_INVOKABLE void moveInProgressToWantToRead();
  Q_INVOKABLE bool hasCachedProgress() const;
  Q_INVOKABLE void restoreCachedProgress();
  Q_INVOKABLE void discardCachedProgress() const;
  Q_INVOKABLE void updateReadingProgress(int pageNumber, int durationSeconds);
  Q_INVOKABLE void deleteReadingSession(const QString &sessionId);

  models::BookSearchProxyModel *searchModel() const;
  models::BookCharactersModel *charactersModel() const;
  models::ReadingHistoryModel *readingHistoryModel() const;

  static BookController *create(QQmlEngine *engine, QJSEngine *scriptEngine);
  static void setInstance(BookController *instance);

signals:
  void currentBookIsbnChanged();
  void errorMessageChanged();
  void bookSaved();
  void activeKindChanged();
  void bookOpenRequested(qint64 isbn);

private:
  void setErrorMessage(const QString &message);

  models::BookSortFilterProxyModel *buildProxy(models::BookListModel *source,
                                               const models::filters::BookFilterStrategy &strategy);
  // Copies the pdf in and renders its cover beside it, then writes both paths
  // onto the book. Metadata only overrides the book's own fields for a custom
  // one — a catalog book keeps what the catalog said.
  bool applyPdf(services::BookDTO &book, const QString &sourcePath, const services::PdfDocumentInfo &info) const;
  static void overrideFromPdf(services::BookDTO &book, const services::PdfDocumentInfo &info);
  // Copies a cover the user picked into the store, so it survives the original
  // being moved. Returns whether the book's coverUrl changed.
  bool adoptPickedCover(services::BookDTO &book) const;
  // Reads the ISBN the form typed, if any. Reports a typed-but-invalid one so it
  // is refused rather than silently dropped.
  bool resolveTypedIsbn(const QVariantMap &fields, qint64 &isbn);
  void applyActiveSourceToSearchProxy();

  static BookController *s_instance;

  std::shared_ptr<services::BookTable> _bookTable;
  std::shared_ptr<services::BookFileStore> _fileStore;
  models::BookListModel *_listModel;
  models::BookSearchProxyModel *_searchProxy;
  models::BookCriteriaFilterProxyModel *_criteriaProxy;
  models::BookCharactersModel *_charactersModel;
  models::ReadingHistoryModel *_readingHistoryModel;

  QHash<ListKind, models::BookSortFilterProxyModel *> _proxies;
  ListKind _activeKind{ListKind::WantToRead};

  QString _errorMessage;
  qint64 _currentBookIsbn{0};

  mutable qmltypes::BookDTOObject _cachedBookData;
  mutable bool _cacheValid{false};

  // Parsed once when the add form picks a file, committed by addCustomBook —
  // the stored file is named after the isbn, which does not exist until then.
  QString _stagedPdfPath;
  services::PdfDocumentInfo _stagedPdf;
};

} // namespace readary::controllers

#endif // READARY_CONTROLLERS_BOOKCONTROLLER_HPP
