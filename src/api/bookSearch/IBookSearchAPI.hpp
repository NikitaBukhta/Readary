#ifndef LIBRARY_IBOOKSEARCHAPI_H
#define LIBRARY_IBOOKSEARCHAPI_H

#include "services/BookDTO.hpp"
#include "services/BookFilterCriteria.hpp"

#include <QList>

namespace readary::api {

struct BookSearchFields {
  qint64 isbn{0};
  QString name;
  QString author;
  int page{1};
  QString language;
  services::BookFilterCriteria criteria;
};

class IBookSearchAPI : public QObject {
  Q_OBJECT
public:
  explicit IBookSearchAPI(QObject *parent = nullptr) : QObject{parent} {}
  ~IBookSearchAPI() override = default;
  virtual void search(const BookSearchFields &params) = 0;
  virtual void searchByISBN(qint64 isbn) = 0;
  virtual void fetchDescription(const QString &workKey) = 0;

signals:
  void searchListUpdated(QList<services::BookDTO> books, bool hasMore);
  void descriptionReady(QString workKey, QString description);
};

} // namespace readary::api

#endif // LIBRARY_IBOOKSEARCHAPI_H
