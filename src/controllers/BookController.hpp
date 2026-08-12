#ifndef READARY_CONTROLLERS_BOOKCONTROLLER_HPP
#define READARY_CONTROLLERS_BOOKCONTROLLER_HPP

#include "models/books/BookCharactersModel.hpp"
#include "models/books/BookCriteriaFilterProxyModel.hpp"
#include "models/books/BookSearchProxyModel.hpp"
#include "models/books/BookSortFilterProxyModel.hpp"
#include "qmltypes/BookDTOObject.hpp"
#include "services/BookTable.hpp"

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

public:
  enum class ListKind : uint8_t {
    WantToRead = 0,
    WantToBuy = 1,
    AlreadyRead = 2,
    InProgress = 3,
  };
  Q_ENUM(ListKind)

  explicit BookController(std::shared_ptr<services::BookTable> bookTable, models::BookListModel *listModel,
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

  models::BookSearchProxyModel *searchModel() const;
  models::BookCharactersModel *charactersModel() const;

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
  void applyActiveSourceToSearchProxy();

  static BookController *s_instance;

  std::shared_ptr<services::BookTable> _bookTable;
  models::BookListModel *_listModel;
  models::BookSearchProxyModel *_searchProxy;
  models::BookCriteriaFilterProxyModel *_criteriaProxy;
  models::BookCharactersModel *_charactersModel;

  QHash<ListKind, models::BookSortFilterProxyModel *> _proxies;
  ListKind _activeKind{ListKind::WantToRead};

  QString _errorMessage;
  qint64 _currentBookIsbn{0};

  mutable qmltypes::BookDTOObject _cachedBookData;
  mutable bool _cacheValid{false};
};

} // namespace readary::controllers

#endif // READARY_CONTROLLERS_BOOKCONTROLLER_HPP
