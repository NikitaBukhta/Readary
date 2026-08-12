#include "core/AppEnvironment.hpp"
#include "core/AppInitializer.hpp"

#include <QGuiApplication>
#include <QLoggingCategory>

using Qt::StringLiterals::operator""_s;

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);

  // Anchor QSettings to a fixed location regardless of binary name / build kind.
  QGuiApplication::setOrganizationName(u"DarieszzBooks"_s);
  QGuiApplication::setOrganizationDomain(u"darieszzbooks.local"_s);
  QGuiApplication::setApplicationName(u"DarieszzBooks"_s);

  // Resources from a static library can be stripped by the linker; force init.
  Q_INIT_RESOURCE(db_scripts);
  Q_INIT_RESOURCE(fonts);
  Q_INIT_RESOURCE(emoji_resources);
#ifdef BL_HAS_TRANSLATIONS
  Q_INIT_RESOURCE(translations);
#endif

  readary::core::AppEnvironment::installFileLogger();

#ifdef QT_NO_DEBUG
  QLoggingCategory::setFilterRules(u"readary.*.debug=false\n"
                                   "readary.*.info=false"_s);
#else
  QLoggingCategory::setFilterRules(u"readary.*.debug=true"_s);
#endif

  readary::core::AppInitializer initializer(app);
  return initializer.run();
}
