#ifndef LIBRARY_GLOBALBOOKSEARCHCONTROLLER_HPP
#define LIBRARY_GLOBALBOOKSEARCHCONTROLLER_HPP

#include "BookDTOObject.hpp"
#include "api/bookSearch/IBookSearchAPI.hpp"
#include "models/books/GlobalBookSearchListModel.hpp"
#include "services/BookDTO.hpp"

#include <QHash>
#include <QJSEngine>
#include <QList>
#include <QQmlEngine>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <optional>

namespace readary {
namespace controllers {

class GlobalBookSearchController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(readary::models::GlobalBookSearchListModel *resultsModel READ resultsModel CONSTANT)
public:
  explicit GlobalBookSearchController(QObject *parent);
  void setBookSearchAPI(api::IBookSearchAPI *api);

  models::GlobalBookSearchListModel *resultsModel() const;

  Q_INVOKABLE void search(const QString &query);
  Q_INVOKABLE void loadMore();
  Q_INVOKABLE void openBook(qint64 isbn);

  static GlobalBookSearchController *create(QQmlEngine *engine, QJSEngine *scriptEngine);
  static void setInstance(GlobalBookSearchController *instance);

signals:
  void bookImportRequested(services::BookDTO book);

private:
  struct CachedSearch {
    QList<services::BookDTO> books;
    int nextPage = 1;
    bool hasMore = true;
  };

  static QString normalizeKey(const QString &query);
  static std::optional<qint64> queryAsIsbn(const QString &query);

  bool tryServeFromMemoryCache(const QString &key);
  bool tryServeFromDiskCache(const QString &key);
  void startFreshSearch(const QString &key);
  void requestPage(const QString &query, int page);

  void onSearchResults(const QList<services::BookDTO> &books, bool hasMore);
  CachedSearch &accumulatePage(const QString &query, int page, const QList<services::BookDTO> &books, bool hasMore);
  void showPage(const QList<services::BookDTO> &books, bool firstPage);

  static GlobalBookSearchController *s_instance;
  api::IBookSearchAPI *_bookSearchAPI;
  models::GlobalBookSearchListModel *_resultsModel;

  QHash<QString, CachedSearch> _searchCache;
  QString _activeQuery;
  QString _pendingQuery;
  int _pendingPage{1};
  bool _loading = false;
};

} // namespace controllers
} // namespace readary

#endif // LIBRARY_GLOBALBOOKSEARCHCONTROLLER_HPP
