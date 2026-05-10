#ifndef BEELIBRARY_CONTROLLERS_BOOKCONTROLLER_HPP
#define BEELIBRARY_CONTROLLERS_BOOKCONTROLLER_HPP

#include "models/books/BookCharactersModel.hpp"
#include "models/books/BookSearchProxyModel.hpp"
#include "models/books/BookSortFilterProxyModel.hpp"
#include "qmltypes/BookDTOObject.hpp"
#include "services/BookTable.hpp"

#include <QHash>
#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>
#include <memory>

namespace bl::models {
class BookListModel;
namespace filters {
class BookFilterStrategy;
}
} // namespace bl::models

namespace bl::controllers {

class BookController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(qint64 currentBookId READ currentBookId WRITE setCurrentBookId NOTIFY currentBookIdChanged)
  Q_PROPERTY(bl::qmltypes::BookDTOObject currentBookData READ currentBookData NOTIFY currentBookIdChanged)
  Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

  Q_PROPERTY(ListKind activeKind READ activeKind WRITE setActiveKind NOTIFY activeKindChanged)
  Q_PROPERTY(bl::models::BookSearchProxyModel *searchModel READ searchModel CONSTANT)
  Q_PROPERTY(bl::models::BookCharactersModel *charactersModel READ charactersModel CONSTANT)

public:
  enum class ListKind {
    WantToRead = 0,
    WantToBuy = 1,
    AlreadyRead = 2,
    InProgress = 3,
  };
  Q_ENUM(ListKind)

  explicit BookController(std::shared_ptr<services::BookTable> bookTable, bl::models::BookListModel *listModel,
                          QObject *parent);
  ~BookController() override;

  qint64 currentBookId() const;
  void setCurrentBookId(qint64 id);
  bl::qmltypes::BookDTOObject currentBookData() const;
  QString errorMessage() const;

  ListKind activeKind() const;
  void setActiveKind(ListKind kind);
  Q_INVOKABLE bl::models::BookSortFilterProxyModel *getSortFilterProxyForKind(ListKind kind) const;

  Q_INVOKABLE void openBook(qint64 id);

  static Q_INVOKABLE void saveReadingSession(qint64 bookId, int seconds, int phase);
  static Q_INVOKABLE QVariantMap takeReadingSession(qint64 bookId);
  static Q_INVOKABLE void clearReadingSession(qint64 bookId);
  Q_INVOKABLE void setBookStatus(int status);
  Q_INVOKABLE void updateReadingProgress(int pageNumber, int durationSeconds);

  bl::models::BookSearchProxyModel *searchModel() const;
  bl::models::BookCharactersModel *charactersModel() const;

  static BookController *create(QQmlEngine *engine, QJSEngine *scriptEngine);
  static void setInstance(BookController *instance);

signals:
  void currentBookIdChanged();
  void errorMessageChanged();
  void bookSaved();
  void activeKindChanged();
  void bookOpenRequested(qint64 id);

private:
  void setErrorMessage(const QString &message);

  bl::models::BookSortFilterProxyModel *buildProxy(bl::models::BookListModel *source,
                                                   const bl::models::filters::BookFilterStrategy &strategy);
  void applyActiveSourceToSearchProxy();

  static BookController *s_instance;

  std::shared_ptr<services::BookTable> _bookTable;
  bl::models::BookListModel *_listModel;
  bl::models::BookSearchProxyModel *_searchProxy;
  bl::models::BookCharactersModel *_charactersModel;

  QHash<ListKind, bl::models::BookSortFilterProxyModel *> _proxies;
  ListKind _activeKind = ListKind::WantToRead;

  QString _errorMessage;
  qint64 _currentBookId = 0;

  mutable bl::qmltypes::BookDTOObject _cachedBookData;
  mutable bool _cacheValid = false;
};

} // namespace bl::controllers

#endif // BEELIBRARY_CONTROLLERS_BOOKCONTROLLER_HPP
