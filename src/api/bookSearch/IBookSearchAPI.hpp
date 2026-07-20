#ifndef LIBRARY_IBOOKSEARCHAPI_H
#define LIBRARY_IBOOKSEARCHAPI_H

#include "services/BookDTO.hpp"

#include <QList>

namespace readary {
namespace api {

struct BookSearchFields {
  qint64 isbn;
  QString name;
  QString author;
  int page{1};
};

class IBookSearchAPI : public QObject {
  Q_OBJECT
public:
  IBookSearchAPI(QObject *parent = nullptr) : QObject{parent} {}
  virtual ~IBookSearchAPI() = default;
  virtual void search(const BookSearchFields &params) = 0;
  virtual void searchByISBN(qint64 isbn) = 0;
  virtual void fetchDescription(const QString &workKey) = 0;

signals:
  void searchListUpdated(QList<services::BookDTO> books, bool hasMore);
  void descriptionReady(QString workKey, QString description);
};

} // namespace api
} // namespace readary

#endif // LIBRARY_IBOOKSEARCHAPI_H
