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

public:
  enum class ListKind {
    WantToRead = 0,
    WantToBuy = 1,
    AlreadyRead = 2,
    InProgress = 3,
  };
  Q_ENUM(ListKind)

  explicit BookController(std::shared_ptr<services::BookTable> bookTable, readary::models::BookListModel *listModel,
                          QObject *parent);
  ~BookController() override;

  qint64 currentBookIsbn() const;
  void setCurrentBookIsbn(qint64 isbn);
  readary::qmltypes::BookDTOObject currentBookData() const;
  QString errorMessage() const;

  ListKind activeKind() const;
  void setActiveKind(ListKind kind);
  Q_INVOKABLE readary::models::BookSortFilterProxyModel *getSortFilterProxyForKind(ListKind kind) const;

  Q_INVOKABLE void openBook(qint64 isbn);

  static Q_INVOKABLE void saveReadingSession(qint64 bookIsbn, int seconds, int phase);
  static Q_INVOKABLE QVariantMap takeReadingSession(qint64 bookIsbn);
  static Q_INVOKABLE void clearReadingSession(qint64 bookIsbn);
  Q_INVOKABLE void setBookStatus(int status);
  Q_INVOKABLE void updateReadingProgress(int pageNumber, int durationSeconds);

  readary::models::BookSearchProxyModel *searchModel() const;
  readary::models::BookCharactersModel *charactersModel() const;

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

  readary::models::BookSortFilterProxyModel *buildProxy(readary::models::BookListModel *source,
                                                   const readary::models::filters::BookFilterStrategy &strategy);
  void applyActiveSourceToSearchProxy();

  static BookController *s_instance;

  std::shared_ptr<services::BookTable> _bookTable;
  readary::models::BookListModel *_listModel;
  readary::models::BookSearchProxyModel *_searchProxy;
  readary::models::BookCharactersModel *_charactersModel;

  QHash<ListKind, readary::models::BookSortFilterProxyModel *> _proxies;
  ListKind _activeKind = ListKind::WantToRead;

  QString _errorMessage;
  qint64 _currentBookIsbn = 0;

  mutable readary::qmltypes::BookDTOObject _cachedBookData;
  mutable bool _cacheValid = false;
};

} // namespace readary::controllers

#endif // BEELIBRARY_CONTROLLERS_BOOKCONTROLLER_HPP
