#ifndef LIBRARY_GLOBALBOOKSEARCHCONTROLLER_HPP
#define LIBRARY_GLOBALBOOKSEARCHCONTROLLER_HPP

#include "BookDTOObject.hpp"
#include "api/bookSearch/IBookSearchAPI.hpp"

#include <QtQml/qqmlregistration.h>
#include <QString>

namespace readary {
namespace controllers {

class GlobalBookSearchController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
public:
  explicit GlobalBookSearchController(QObject *parent = nullptr);
  void setBookSearchAPI(api::IBookSearchAPI* api);
  
  Q_INVOKABLE void search(QString query);

  static void setInstance(GlobalBookSearchController *instance);

private:
  static GlobalBookSearchController *s_instance;
  api::IBookSearchAPI* _bookSearchAPI;
};

} // controllers
} // readary

#endif //LIBRARY_GLOBALBOOKSEARCHCONTROLLER_HPP
