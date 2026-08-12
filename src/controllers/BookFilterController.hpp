#ifndef READARY_CONTROLLERS_BOOKFILTERCONTROLLER_HPP
#define READARY_CONTROLLERS_BOOKFILTERCONTROLLER_HPP

#include "services/BookFilterCriteria.hpp"

#include <QObject>
#include <QQmlEngine>

namespace readary::models {
class BookListModelBase;
}

namespace readary::controllers {

class BookFilterController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(Scope scope READ scope WRITE setScope NOTIFY scopeChanged)
  Q_PROPERTY(int activeCount READ activeCount NOTIFY criteriaApplied)
  Q_PROPERTY(int draftCount READ draftCount NOTIFY draftChanged)
  Q_PROPERTY(QStringList availableLanguages READ availableLanguages NOTIFY facetsChanged)
  Q_PROPERTY(QStringList availableGenres READ availableGenres NOTIFY facetsChanged)
  Q_PROPERTY(QStringList availablePublishers READ availablePublishers NOTIFY facetsChanged)
  Q_PROPERTY(QStringList availableTypes READ availableTypes NOTIFY facetsChanged)
  Q_PROPERTY(QStringList selectedLanguages READ selectedLanguages NOTIFY draftChanged)
  Q_PROPERTY(QStringList selectedGenres READ selectedGenres NOTIFY draftChanged)
  Q_PROPERTY(QStringList selectedTypes READ selectedTypes NOTIFY draftChanged)
  Q_PROPERTY(QString author READ author WRITE setAuthor NOTIFY draftChanged)
  Q_PROPERTY(QString publisher READ publisher WRITE setPublisher NOTIFY draftChanged)
  Q_PROPERTY(int minPages READ minPages WRITE setMinPages NOTIFY draftChanged)
  Q_PROPERTY(int maxPages READ maxPages WRITE setMaxPages NOTIFY draftChanged)
  Q_PROPERTY(int minYear READ minYear WRITE setMinYear NOTIFY draftChanged)
  Q_PROPERTY(int maxYear READ maxYear WRITE setMaxYear NOTIFY draftChanged)
  Q_PROPERTY(double minRating READ minRating WRITE setMinRating NOTIFY draftChanged)

public:
  enum class Scope : std::uint8_t {
    Library = 0,
    Search = 1,
  };
  Q_ENUM(Scope)

  explicit BookFilterController(QObject *parent);

  void setLibraryModel(models::BookListModelBase *model);
  void setSearchModel(models::BookListModelBase *model);

  Scope scope() const;
  void setScope(Scope scope);

  const services::BookFilterCriteria &criteria() const;
  int activeCount() const;
  int draftCount() const;

  QStringList availableLanguages() const;
  QStringList availableGenres() const;
  QStringList availablePublishers() const;
  QStringList availableTypes() const;

  QStringList selectedLanguages() const;
  QStringList selectedGenres() const;
  QStringList selectedTypes() const;
  QString author() const;
  QString publisher() const;
  int minPages() const;
  int maxPages() const;
  int minYear() const;
  int maxYear() const;
  double minRating() const;

  void setAuthor(const QString &author);
  void setPublisher(const QString &publisher);
  void setMinPages(int pages);
  void setMaxPages(int pages);
  void setMinYear(int year);
  void setMaxYear(int year);
  void setMinRating(double rating);

  Q_INVOKABLE void toggleLanguage(const QString &code);
  Q_INVOKABLE void toggleGenre(const QString &name);
  Q_INVOKABLE void toggleType(const QString &name);

  Q_INVOKABLE void syncDraft();
  Q_INVOKABLE void apply();
  Q_INVOKABLE void reset();

  Q_INVOKABLE static QString languageLabel(const QString &code);

  static BookFilterController *create(QQmlEngine *engine, QJSEngine *scriptEngine);
  static void setInstance(BookFilterController *instance);

signals:
  void scopeChanged();
  void draftChanged();
  void facetsChanged();
  void criteriaApplied();

private:
  models::BookListModelBase *facetSource() const;
  void setBound(int &bound, int value);
  static void toggle(QStringList &list, const QString &value);

  static BookFilterController *s_instance;

  models::BookListModelBase *_libraryModel{nullptr};
  models::BookListModelBase *_searchModel{nullptr};
  Scope _scope{Scope::Library};

  services::BookFilterCriteria _draft;
  services::BookFilterCriteria _applied;
};

} // namespace readary::controllers

#endif // READARY_CONTROLLERS_BOOKFILTERCONTROLLER_HPP
