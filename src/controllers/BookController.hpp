#ifndef BEELIBRARY_CONTROLLERS_BOOKCONTROLLER_HPP
#define BEELIBRARY_CONTROLLERS_BOOKCONTROLLER_HPP

#include "models/BookSearchProxyModel.hpp"
#include "models/BookSortFilterProxyModel.hpp"
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

  Q_PROPERTY(int currentBookId READ currentBookId WRITE setCurrentBookId NOTIFY currentBookIdChanged)
  Q_PROPERTY(QVariantMap currentBookData READ currentBookData NOTIFY currentBookIdChanged)
  Q_PROPERTY(bool editMode READ editMode NOTIFY currentBookIdChanged)
  Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
  Q_PROPERTY(int yearMin READ yearMin CONSTANT)
  Q_PROPERTY(int yearMax READ yearMax CONSTANT)

  Q_PROPERTY(ListKind activeKind READ activeKind WRITE setActiveKind NOTIFY activeKindChanged)
  Q_PROPERTY(bl::models::BookSearchProxyModel *searchModel READ searchModel CONSTANT)
  Q_PROPERTY(bl::models::BookSortFilterProxyModel *wantToReadModel READ wantToReadModel CONSTANT)
  Q_PROPERTY(bl::models::BookSortFilterProxyModel *wantToBuyModel READ wantToBuyModel CONSTANT)
  Q_PROPERTY(bl::models::BookSortFilterProxyModel *alreadyReadModel READ alreadyReadModel CONSTANT)
  Q_PROPERTY(bl::models::BookSortFilterProxyModel *readInProgressModel READ readInProgressModel CONSTANT)
  Q_PROPERTY(bl::models::BookSortFilterProxyModel *activeModel READ activeModel NOTIFY activeKindChanged)

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

  // Form
  int currentBookId() const;
  void setCurrentBookId(int id);
  QVariantMap currentBookData() const;
  bool editMode() const;
  QString errorMessage() const;
  int yearMin() const;
  int yearMax() const;

  // List
  ListKind activeKind() const;
  void setActiveKind(ListKind kind);

  bl::models::BookSearchProxyModel *searchModel() const;
  bl::models::BookSortFilterProxyModel *wantToReadModel() const;
  bl::models::BookSortFilterProxyModel *wantToBuyModel() const;
  bl::models::BookSortFilterProxyModel *alreadyReadModel() const;
  bl::models::BookSortFilterProxyModel *readInProgressModel() const;
  bl::models::BookSortFilterProxyModel *activeModel() const;

  static BookController *create(QQmlEngine *engine, QJSEngine *scriptEngine);
  static void setInstance(BookController *instance);

signals:
  void currentBookIdChanged();
  void errorMessageChanged();
  void bookSaved();
  void activeKindChanged();

private:
  static QString normalizeIsbn(const QString &rawIsbn);
  static QString stripIsbnPrefix(const QString &isbn);

  bool validate(const QString &title, const QString &author, int year, const QString &isbn);
  void setErrorMessage(const QString &message);

  bl::models::BookSortFilterProxyModel *buildProxy(bl::models::BookListModel *source,
                                                   const bl::models::filters::BookFilterStrategy &strategy);
  bl::models::BookSortFilterProxyModel *proxyFor(ListKind kind) const;
  void applyActiveSourceToSearchProxy();

  static BookController *s_instance;

  std::shared_ptr<services::BookTable> _bookTable;
  bl::models::BookListModel *_listModel;
  bl::models::BookSearchProxyModel *_searchProxy;

  QHash<ListKind, bl::models::BookSortFilterProxyModel *> _proxies;
  ListKind _activeKind = ListKind::WantToRead;

  QString _errorMessage;
  int _currentBookId = 0;
};

} // namespace bl::controllers

#endif // BEELIBRARY_CONTROLLERS_BOOKCONTROLLER_HPP
