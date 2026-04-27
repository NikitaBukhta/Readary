#include "core/AppEnvironment.hpp"
#include "core/AppInitializer.hpp"

#include <QGuiApplication>
#include <QLoggingCategory>

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);

  // Resources from a static library can be stripped by the linker; force init.
  Q_INIT_RESOURCE(db_scripts);

  bl::core::AppEnvironment::installFileLogger();

#ifdef QT_NO_DEBUG
  QLoggingCategory::setFilterRules("bl.*.debug=false\n"
                                   "bl.*.info=false");
#else
  QLoggingCategory::setFilterRules("bl.*.debug=true");
#endif

  bl::core::AppInitializer initializer(app);
  return initializer.run();
}
