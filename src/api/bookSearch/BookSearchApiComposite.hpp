#ifndef LIBRARY_BOOKSEARCHAPICOMPOSITE_HPP
#define LIBRARY_BOOKSEARCHAPICOMPOSITE_HPP

#include "IBookSearchAPI.hpp"
#include <QList>

namespace readary::api {

class BookSearchAPIComposite : public IBookSearchAPI {
  Q_OBJECT
public:
  explicit BookSearchAPIComposite(QList<IBookSearchAPI *> &&bookSearchAPIs, QObject *parent = nullptr);

  void setFallbackAPI(IBookSearchAPI *fallback);
  void search(const BookSearchFields &params) override;
  void searchByISBN(qint64 isbn) override;
  void fetchDescription(const QString &workKey) override;

private:
  void handlePrimaryResult(const QList<services::BookDTO> &rawBooks, bool hasMore);
  void handleFallbackResult(const QList<services::BookDTO> &rawBooks, bool hasMore);

  enum class Stage : uint8_t { Idle, AwaitingPrimary, AwaitingFallback };

  void initConnect();
  QList<services::BookDTO> applyCriteria(const QList<services::BookDTO> &books) const;
  void beginSearch(const BookSearchFields &params, bool byIsbn);
  void dispatch(IBookSearchAPI *bookAPI) const;
  void finish();

  QList<IBookSearchAPI *> _bookSearchAPIs;
  IBookSearchAPI *_fallbackAPI{nullptr};

  QList<services::BookDTO> _aggregated;
  BookSearchFields _params{};
  bool _byIsbn{false};
  bool _hasMore{false};
  int _pendingPrimary{0};
  Stage _stage{Stage::Idle};
};

} // namespace readary::api

#endif // LIBRARY_BOOKSEARCHAPICOMPOSITE_HPP
