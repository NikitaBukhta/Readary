#include "core/AppEnvironment.hpp"
#include "core/AppInitializer.hpp"

#include <QGuiApplication>
#include <QLoggingCategory>

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);

  bl::core::AppEnvironment::installFileLogger();

#ifdef QT_NO_DEBUG
  QLoggingCategory::setFilterRules("bl.*.debug=false\n"
                                   "bl.*.info=false");
#endif

  bl::core::AppInitializer initializer(app);
  return initializer.run();
}
