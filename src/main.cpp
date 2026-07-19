#include "core/AppEnvironment.hpp"
#include "core/AppInitializer.hpp"

#include <QGuiApplication>
#include <QLoggingCategory>

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);

  // Anchor QSettings to a fixed location regardless of binary name / build kind.
  QGuiApplication::setOrganizationName("DarieszzBooks");
  QGuiApplication::setOrganizationDomain("darieszzbooks.local");
  QGuiApplication::setApplicationName("DarieszzBooks");

  // Resources from a static library can be stripped by the linker; force init.
  Q_INIT_RESOURCE(db_scripts);
  Q_INIT_RESOURCE(fonts);
  Q_INIT_RESOURCE(emoji_resources);
#ifdef BL_HAS_TRANSLATIONS
  Q_INIT_RESOURCE(translations);
#endif

  readary::core::AppEnvironment::installFileLogger();

#ifdef QT_NO_DEBUG
  QLoggingCategory::setFilterRules("readary.*.debug=false\n"
                                   "readary.*.info=false");
#else
  QLoggingCategory::setFilterRules("readary.*.debug=true");
#endif

  readary::core::AppInitializer initializer(app);
  return initializer.run();
}
