#ifndef LIBRARY_GLOBALBOOKSEARCHCONTROLLER_HPP
#define LIBRARY_GLOBALBOOKSEARCHCONTROLLER_HPP

#include "BookDTOObject.hpp"
#include "api/bookSearch/IBookSearchAPI.hpp"
#include "api/translate/ITranslator.hpp"
#include "models/books/GlobalBookSearchListModel.hpp"
#include "models/settings/LanguageModel.hpp"
#include "services/BookDTO.hpp"

#include <QHash>
#include <QJSEngine>
#include <QList>
#include <QQmlEngine>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <functional>
#include <optional>

namespace readary::controllers {

class GlobalBookSearchController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(readary::models::GlobalBookSearchListModel *resultsModel READ resultsModel CONSTANT)
public:
  explicit GlobalBookSearchController(QObject *parent);
  void setBookSearchAPI(api::IBookSearchAPI *api);
  void setTranslator(api::ITranslator *translator);
  void setLanguageModel(models::LanguageModel *languageModel);
  void setOwnershipChecker(std::function<bool(qint64)> isOwned);
  void setFilterCriteria(const services::BookFilterCriteria &criteria);

  models::GlobalBookSearchListModel *resultsModel() const;

  Q_INVOKABLE void search(const QString &query);
  Q_INVOKABLE void setPendingQuery(const QString &text);
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

  QString cacheKey(const QString &normalizedQuery) const;

  bool tryServeFromMemoryCache(const QString &key);
  bool tryServeFromDiskCache(const QString &key);
  void startFreshSearch(const QString &key);
  void requestPage(const QString &query, int page);
  void maybeFetchMorePages(const CachedSearch &entry, bool hasMore);

  void onSearchResults(const QList<services::BookDTO> &books, bool hasMore);
  CachedSearch &accumulatePage(const QString &key, int page, const QList<services::BookDTO> &books, bool hasMore);
  void showPage(const QList<services::BookDTO> &books, bool firstPage);

  void onDescriptionReady(const QString &workKey, const QString &description);
  bool maybeTranslateDescription(const QString &description);
  void onTranslationReady(quint64 requestId, const QString &translated);

  static GlobalBookSearchController *s_instance;
  api::IBookSearchAPI *_bookSearchAPI{nullptr};
  api::ITranslator *_translator{nullptr};
  models::LanguageModel *_languageModel{nullptr};
  models::GlobalBookSearchListModel *_resultsModel;
  std::function<bool(qint64)> _isOwned;

  QHash<QString, CachedSearch> _searchCache;
  services::BookFilterCriteria _criteria;
  QString _activeQuery;
  QString _activeKey;
  QString _pendingKey;
  int _pendingPage{1};
  int _autoPagesFetched{0};
  bool _loading{false};

  services::BookDTO _pendingImport;
  quint64 _nextTranslateId{0};
  quint64 _pendingTranslateId{0};
};

} // namespace readary::controllers

#endif // LIBRARY_GLOBALBOOKSEARCHCONTROLLER_HPP
