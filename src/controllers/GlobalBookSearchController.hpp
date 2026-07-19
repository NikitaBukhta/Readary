#ifndef LIBRARY_GLOBALBOOKSEARCHCONTROLLER_HPP
#define LIBRARY_GLOBALBOOKSEARCHCONTROLLER_HPP

#include "BookDTOObject.hpp"
#include "api/bookSearch/IBookSearchAPI.hpp"
#include "models/books/GlobalBookSearchListModel.hpp"
#include "services/BookDTO.hpp"

#include <QJSEngine>
#include <QQmlEngine>
#include <QString>
#include <QtQml/qqmlregistration.h>

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

  Q_INVOKABLE void search(const QString &query) const;
  Q_INVOKABLE void openBook(qint64 isbn);

  static GlobalBookSearchController *create(QQmlEngine *engine, QJSEngine *scriptEngine);
  static void setInstance(GlobalBookSearchController *instance);

signals:
  void bookImportRequested(services::BookDTO book);

private:
  static GlobalBookSearchController *s_instance;
  api::IBookSearchAPI *_bookSearchAPI;
  models::GlobalBookSearchListModel *_resultsModel;
};

} // namespace controllers
} // namespace readary

#endif // LIBRARY_GLOBALBOOKSEARCHCONTROLLER_HPP
