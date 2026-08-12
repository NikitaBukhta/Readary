#include "BookFilterController.hpp"

#include "api/translate/LanguageConverter.hpp"
#include "models/books/BookListModelBase.hpp"
#include "models/settings/LanguageModel.hpp"

#include <QLocale>
#include <QLoggingCategory>
#include <algorithm>
#include <utility>

namespace {
Q_LOGGING_CATEGORY(lcFilter, "readary.controllers.filter")

void collectDistinct(QStringList &into, const QString &value) {
  const QString trimmed = value.trimmed();
  if (trimmed.isEmpty() || into.contains(trimmed, Qt::CaseInsensitive)) {
    return;
  }
  into.append(trimmed);
}

template <typename Extract>
QStringList collectFacet(const readary::models::BookListModelBase *source, Extract extract, QStringList seed = {}) {
  QStringList result = std::move(seed);
  if (source != nullptr) {
    for (const auto &book : source->books()) {
      extract(result, book);
    }
  }
  result.sort(Qt::CaseInsensitive);
  return result;
}

} // namespace

namespace readary::controllers {

BookFilterController *BookFilterController::s_instance = nullptr;

BookFilterController::BookFilterController(QObject *parent) : QObject{parent} {}

void BookFilterController::setLibraryModel(models::BookListModelBase *model) {
  _libraryModel = model;
  if (_libraryModel != nullptr) {
    connect(_libraryModel, &QAbstractItemModel::modelReset, this, &BookFilterController::facetsChanged);
  }
}

void BookFilterController::setSearchModel(models::BookListModelBase *model) {
  _searchModel = model;
  if (_searchModel != nullptr) {
    connect(_searchModel, &QAbstractItemModel::modelReset, this, &BookFilterController::facetsChanged);
    connect(_searchModel, &QAbstractItemModel::rowsInserted, this, &BookFilterController::facetsChanged);
  }
}

BookFilterController::Scope BookFilterController::scope() const { return _scope; }

void BookFilterController::setScope(Scope scope) {
  if (_scope == scope) {
    return;
  }
  _scope = scope;
  emit scopeChanged();
  emit facetsChanged();
}

models::BookListModelBase *BookFilterController::facetSource() const {
  return _scope == Scope::Search ? _searchModel : _libraryModel;
}

const services::BookFilterCriteria &BookFilterController::criteria() const { return _applied; }

int BookFilterController::activeCount() const { return _applied.activeCount(); }

int BookFilterController::draftCount() const { return _draft.activeCount(); }

QStringList BookFilterController::availableLanguages() const {
  QStringList appLanguages;
  for (const int raw : models::LanguageModel::available()) {
    collectDistinct(appLanguages, models::LanguageModel::localeCode(static_cast<models::LanguageModel::Code>(raw)));
  }

  return collectFacet(
      facetSource(),
      [](QStringList &into, const services::BookDTO &book) {
        // A cell may list several languages ("en, fr") — each is its own facet.
        for (const QString &code : book.language.split(u',', Qt::SkipEmptyParts)) {
          collectDistinct(into, code);
        }
      },
      std::move(appLanguages));
}

QStringList BookFilterController::availableGenres() const {
  return collectFacet(facetSource(), [](QStringList &into, const services::BookDTO &book) {
    for (const QString &genre : book.genres) {
      collectDistinct(into, genre);
    }
  });
}

QStringList BookFilterController::availablePublishers() const {
  return collectFacet(facetSource(), [](QStringList &into, const services::BookDTO &book) {
    collectDistinct(into, book.publisherName);
  });
}

QStringList BookFilterController::availableTypes() const {
  return collectFacet(facetSource(),
                      [](QStringList &into, const services::BookDTO &book) { collectDistinct(into, book.typeName); });
}

QStringList BookFilterController::selectedLanguages() const { return _draft.languages; }
QStringList BookFilterController::selectedGenres() const { return _draft.genres; }
QStringList BookFilterController::selectedTypes() const { return _draft.types; }
QString BookFilterController::author() const { return _draft.author; }
QString BookFilterController::publisher() const { return _draft.publisher; }
int BookFilterController::minPages() const { return _draft.minPages; }
int BookFilterController::maxPages() const { return _draft.maxPages; }
int BookFilterController::minYear() const { return _draft.minYear; }
int BookFilterController::maxYear() const { return _draft.maxYear; }
double BookFilterController::minRating() const { return _draft.minRating; }

void BookFilterController::setAuthor(const QString &author) {
  if (_draft.author == author) {
    return;
  }
  _draft.author = author;
  emit draftChanged();
}

void BookFilterController::setPublisher(const QString &publisher) {
  if (_draft.publisher == publisher) {
    return;
  }
  _draft.publisher = publisher;
  emit draftChanged();
}

void BookFilterController::setMinPages(int pages) { setBound(_draft.minPages, pages); }

void BookFilterController::setMaxPages(int pages) { setBound(_draft.maxPages, pages); }

void BookFilterController::setMinYear(int year) { setBound(_draft.minYear, year); }

void BookFilterController::setMaxYear(int year) { setBound(_draft.maxYear, year); }

void BookFilterController::setBound(int &bound, int value) {
  const int clamped = std::max(0, value);
  if (bound == clamped) {
    return;
  }
  bound = clamped;
  emit draftChanged();
}

void BookFilterController::setMinRating(double rating) {
  const double clamped = std::max(0.0, rating);
  if (qFuzzyIsNull(_draft.minRating - clamped)) {
    return;
  }
  _draft.minRating = clamped;
  emit draftChanged();
}

void BookFilterController::toggle(QStringList &list, const QString &value) {
  const QString trimmed = value.trimmed();
  if (trimmed.isEmpty()) {
    return;
  }
  const auto it = std::ranges::find_if(
      list, [&trimmed](const QString &entry) { return entry.compare(trimmed, Qt::CaseInsensitive) == 0; });
  if (it != list.end()) {
    list.erase(it);
  } else {
    list.append(trimmed);
  }
}

void BookFilterController::toggleLanguage(const QString &code) {
  toggle(_draft.languages, code);
  emit draftChanged();
}

void BookFilterController::toggleGenre(const QString &name) {
  toggle(_draft.genres, name);
  emit draftChanged();
}

void BookFilterController::toggleType(const QString &name) {
  toggle(_draft.types, name);
  emit draftChanged();
}

void BookFilterController::syncDraft() {
  _draft = _applied;
  emit draftChanged();
}

void BookFilterController::apply() {
  _applied = _draft;
  qCInfo(lcFilter) << "criteria applied — scope:" << (_scope == Scope::Search ? "search" : "library")
                   << "active criteria:" << _applied.activeCount();
  emit criteriaApplied();
}

void BookFilterController::reset() {
  _draft = {};
  _applied = {};
  qCInfo(lcFilter) << "criteria reset";
  emit draftChanged();
  emit criteriaApplied();
}

QString BookFilterController::languageLabel(const QString &code) {
  const QLocale::Language language = api::LanguageConverter::fromCode(code);
  if (language == QLocale::AnyLanguage) {
    return code;
  }
  return QLocale::languageToString(language);
}

BookFilterController *BookFilterController::create(QQmlEngine *engine, QJSEngine *scriptEngine) {
  Q_UNUSED(engine)
  Q_UNUSED(scriptEngine)
  Q_ASSERT_X(s_instance, "BookFilterController::create", "setInstance() must be called before the QML engine loads");
  QQmlEngine::setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
  return s_instance;
}

void BookFilterController::setInstance(BookFilterController *instance) { s_instance = instance; }

} // namespace readary::controllers
