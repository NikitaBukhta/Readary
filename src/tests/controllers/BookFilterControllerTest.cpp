#include "controllers/BookFilterController.hpp"
#include "models/books/GlobalBookSearchListModel.hpp"
#include "services/BookDTO.hpp"

#include <QLocale>
#include <QSignalSpy>
#include <QTest>

#include <memory>

using Qt::StringLiterals::operator""_s;

using readary::controllers::BookFilterController;
using readary::models::GlobalBookSearchListModel;
using readary::services::BookDTO;

namespace {

using Scope = BookFilterController::Scope;

BookDTO makeBook(const QString &name, const QString &author, const QString &publisher, const QString &language,
                 const QString &type, const QStringList &genres) {
  BookDTO book;
  book.name = name;
  book.authorName = author;
  book.publisherName = publisher;
  book.language = language;
  book.typeName = type;
  book.genres = genres;
  return book;
}

} // namespace

class BookFilterControllerTest : public QObject {
  Q_OBJECT

private slots:
  void init();

  void scope_defaultsToLibrary();
  void setScope_announcesNewFacets();
  void setScope_toTheSameScope_isANoOp();
  void facets_followTheActiveScope();

  void availableLanguages_seedTheUiLanguages();
  void availableLanguages_splitACsvCell();
  void availableGenres_areDistinctAndSorted();
  void availablePublishers_areDistinctAndSorted();
  void availableTypes_areDistinctAndSorted();
  void facets_ignoreBlankValues();
  void facets_withoutAModel_stillListTheUiLanguages();

  void toggleLanguage_addsThenRemoves();
  void toggleGenre_addsThenRemoves();
  void toggleType_addsThenRemoves();
  void toggle_isCaseInsensitive();
  void toggle_trimsTheValue();
  void toggle_ignoresBlanks();

  void setAuthor_updatesTheDraft();
  void setAuthor_toTheSameValue_isANoOp();
  void setPublisher_updatesTheDraft();
  void setBounds_clampNegativesToZero();
  void setBounds_toTheSameValue_areNoOps();
  void setMinRating_clampsNegativesToZero();
  void setMinRating_toTheSameValue_isANoOp();

  void draftCount_countsWhatIsPending();
  void apply_promotesTheDraft();
  void syncDraft_takesTheAppliedSetBack();
  void reset_clearsBothSets();

  void languageLabel_namesAKnownCode();
  void languageLabel_unknownCode_isEchoedBack();

private:
  std::unique_ptr<BookFilterController> _controller;
  GlobalBookSearchListModel _libraryModel;
  GlobalBookSearchListModel _searchModel;
};

void BookFilterControllerTest::init() {
  _libraryModel.setBooks({
      makeBook(u"Dune"_s, u"Frank Herbert"_s, u"Ace Books"_s, u"en"_s, u"paper"_s, {u"Science Fiction"_s}),
      makeBook(u"Refactoring"_s, u"Martin Fowler"_s, u"Addison-Wesley"_s, u"en, de"_s, u"ebook"_s, {u"Software"_s}),
  });
  _searchModel.setBooks({
      makeBook(u"Solaris"_s, u"Stanisław Lem"_s, u"Wydawnictwo"_s, u"pl"_s, u"audio"_s, {u"Classics"_s}),
  });

  // A fresh controller per test: draft and applied criteria are its whole state.
  _controller = std::make_unique<BookFilterController>(nullptr);
  _controller->setLibraryModel(&_libraryModel);
  _controller->setSearchModel(&_searchModel);
}

void BookFilterControllerTest::scope_defaultsToLibrary() { QCOMPARE(_controller->scope(), Scope::Library); }

void BookFilterControllerTest::setScope_announcesNewFacets() {
  QSignalSpy scopeSpy{_controller.get(), &BookFilterController::scopeChanged};
  QSignalSpy facetSpy{_controller.get(), &BookFilterController::facetsChanged};

  _controller->setScope(Scope::Search);

  QCOMPARE(scopeSpy.count(), 1);
  QCOMPARE(facetSpy.count(), 1);
  QCOMPARE(_controller->scope(), Scope::Search);
}

void BookFilterControllerTest::setScope_toTheSameScope_isANoOp() {
  QSignalSpy scopeSpy{_controller.get(), &BookFilterController::scopeChanged};

  _controller->setScope(Scope::Library);

  QCOMPARE(scopeSpy.count(), 0);
}

void BookFilterControllerTest::facets_followTheActiveScope() {
  QCOMPARE(_controller->availableTypes(), QStringList({u"ebook"_s, u"paper"_s}));

  _controller->setScope(Scope::Search);

  QCOMPARE(_controller->availableTypes(), QStringList({u"audio"_s}));
}

void BookFilterControllerTest::availableLanguages_seedTheUiLanguages() {
  // The UI languages are always offered, even for a library that has no book
  // in them.
  const QStringList languages = _controller->availableLanguages();

  QVERIFY(languages.contains(u"en"_s));
  QVERIFY(languages.contains(u"ru"_s));
  QVERIFY(languages.contains(u"uk"_s));
}

void BookFilterControllerTest::availableLanguages_splitACsvCell() {
  // "en, de" is two facets, not one.
  QVERIFY(_controller->availableLanguages().contains(u"de"_s));
}

void BookFilterControllerTest::availableGenres_areDistinctAndSorted() {
  _libraryModel.setBooks({
      makeBook(u"Dune"_s, u"Herbert"_s, u"Ace"_s, u"en"_s, u"paper"_s, {u"Science Fiction"_s, u"Classics"_s}),
      makeBook(u"Solaris"_s, u"Lem"_s, u"Ace"_s, u"pl"_s, u"paper"_s, {u"science fiction"_s}),
  });

  QCOMPARE(_controller->availableGenres(), QStringList({u"Classics"_s, u"Science Fiction"_s}));
}

void BookFilterControllerTest::availablePublishers_areDistinctAndSorted() {
  QCOMPARE(_controller->availablePublishers(), QStringList({u"Ace Books"_s, u"Addison-Wesley"_s}));
}

void BookFilterControllerTest::availableTypes_areDistinctAndSorted() {
  QCOMPARE(_controller->availableTypes(), QStringList({u"ebook"_s, u"paper"_s}));
}

void BookFilterControllerTest::facets_ignoreBlankValues() {
  _libraryModel.setBooks({
      makeBook(u"Untyped"_s, u"Nobody"_s, QString{}, u"en"_s, u"   "_s, {}),
  });

  QVERIFY(_controller->availableTypes().isEmpty());
  QVERIFY(_controller->availablePublishers().isEmpty());
  QVERIFY(_controller->availableGenres().isEmpty());
}

void BookFilterControllerTest::facets_withoutAModel_stillListTheUiLanguages() {
  BookFilterController bare{nullptr};

  QVERIFY(bare.availableGenres().isEmpty());
  QVERIFY(bare.availableTypes().isEmpty());
  QVERIFY(bare.availableLanguages().contains(u"en"_s));
}

void BookFilterControllerTest::toggleLanguage_addsThenRemoves() {
  QSignalSpy draftSpy{_controller.get(), &BookFilterController::draftChanged};

  _controller->toggleLanguage(u"en"_s);
  QCOMPARE(_controller->selectedLanguages(), QStringList({u"en"_s}));

  _controller->toggleLanguage(u"en"_s);
  QVERIFY(_controller->selectedLanguages().isEmpty());
  QCOMPARE(draftSpy.count(), 2);
}

void BookFilterControllerTest::toggleGenre_addsThenRemoves() {
  _controller->toggleGenre(u"Software"_s);
  QCOMPARE(_controller->selectedGenres(), QStringList({u"Software"_s}));

  _controller->toggleGenre(u"Software"_s);
  QVERIFY(_controller->selectedGenres().isEmpty());
}

void BookFilterControllerTest::toggleType_addsThenRemoves() {
  _controller->toggleType(u"ebook"_s);
  QCOMPARE(_controller->selectedTypes(), QStringList({u"ebook"_s}));

  _controller->toggleType(u"ebook"_s);
  QVERIFY(_controller->selectedTypes().isEmpty());
}

void BookFilterControllerTest::toggle_isCaseInsensitive() {
  // The facet list and the tapped chip can differ in case.
  _controller->toggleGenre(u"Software"_s);
  _controller->toggleGenre(u"SOFTWARE"_s);

  QVERIFY(_controller->selectedGenres().isEmpty());
}

void BookFilterControllerTest::toggle_trimsTheValue() {
  _controller->toggleGenre(u"  Software  "_s);

  QCOMPARE(_controller->selectedGenres(), QStringList({u"Software"_s}));
}

void BookFilterControllerTest::toggle_ignoresBlanks() {
  _controller->toggleGenre(u"   "_s);
  _controller->toggleLanguage(QString{});

  QVERIFY(_controller->selectedGenres().isEmpty());
  QVERIFY(_controller->selectedLanguages().isEmpty());
}

void BookFilterControllerTest::setAuthor_updatesTheDraft() {
  QSignalSpy draftSpy{_controller.get(), &BookFilterController::draftChanged};

  _controller->setAuthor(u"Herbert"_s);

  QCOMPARE(_controller->author(), u"Herbert"_s);
  QCOMPARE(draftSpy.count(), 1);
}

void BookFilterControllerTest::setAuthor_toTheSameValue_isANoOp() {
  _controller->setAuthor(u"Herbert"_s);

  QSignalSpy draftSpy{_controller.get(), &BookFilterController::draftChanged};
  _controller->setAuthor(u"Herbert"_s);

  QCOMPARE(draftSpy.count(), 0);
}

void BookFilterControllerTest::setPublisher_updatesTheDraft() {
  _controller->setPublisher(u"Ace"_s);
  QCOMPARE(_controller->publisher(), u"Ace"_s);
}

void BookFilterControllerTest::setBounds_clampNegativesToZero() {
  // Zero is the "unbounded" marker, so a negative has to land there.
  _controller->setMinPages(-10);
  _controller->setMaxPages(-10);
  _controller->setMinYear(-10);
  _controller->setMaxYear(-10);

  QCOMPARE(_controller->minPages(), 0);
  QCOMPARE(_controller->maxPages(), 0);
  QCOMPARE(_controller->minYear(), 0);
  QCOMPARE(_controller->maxYear(), 0);
}

void BookFilterControllerTest::setBounds_toTheSameValue_areNoOps() {
  _controller->setMinPages(100);

  QSignalSpy draftSpy{_controller.get(), &BookFilterController::draftChanged};
  _controller->setMinPages(100);

  QCOMPARE(draftSpy.count(), 0);
  QCOMPARE(_controller->minPages(), 100);
}

void BookFilterControllerTest::setMinRating_clampsNegativesToZero() {
  _controller->setMinRating(-1.0);
  QCOMPARE(_controller->minRating(), 0.0);

  _controller->setMinRating(4.5);
  QCOMPARE(_controller->minRating(), 4.5);
}

void BookFilterControllerTest::setMinRating_toTheSameValue_isANoOp() {
  _controller->setMinRating(4.5);

  QSignalSpy draftSpy{_controller.get(), &BookFilterController::draftChanged};
  _controller->setMinRating(4.5);

  QCOMPARE(draftSpy.count(), 0);
}

void BookFilterControllerTest::draftCount_countsWhatIsPending() {
  _controller->toggleGenre(u"Software"_s);
  _controller->setAuthor(u"Fowler"_s);
  // One range counts once, not once per bound.
  _controller->setMinPages(100);
  _controller->setMaxPages(500);

  QCOMPARE(_controller->draftCount(), 3);
  QCOMPARE(_controller->activeCount(), 0);
}

void BookFilterControllerTest::apply_promotesTheDraft() {
  QSignalSpy appliedSpy{_controller.get(), &BookFilterController::criteriaApplied};
  _controller->toggleGenre(u"Software"_s);

  _controller->apply();

  QCOMPARE(appliedSpy.count(), 1);
  QCOMPARE(_controller->activeCount(), 1);
  QCOMPARE(_controller->criteria().genres, QStringList({u"Software"_s}));
}

void BookFilterControllerTest::syncDraft_takesTheAppliedSetBack() {
  // Reopening the sheet after editing without applying must show what is live.
  _controller->toggleGenre(u"Software"_s);
  _controller->apply();
  _controller->setAuthor(u"Abandoned"_s);

  _controller->syncDraft();

  QVERIFY(_controller->author().isEmpty());
  QCOMPARE(_controller->selectedGenres(), QStringList({u"Software"_s}));
}

void BookFilterControllerTest::reset_clearsBothSets() {
  _controller->toggleGenre(u"Software"_s);
  _controller->apply();

  QSignalSpy draftSpy{_controller.get(), &BookFilterController::draftChanged};
  QSignalSpy appliedSpy{_controller.get(), &BookFilterController::criteriaApplied};
  (*_controller).reset();

  QCOMPARE(_controller->draftCount(), 0);
  QCOMPARE(_controller->activeCount(), 0);
  QCOMPARE(draftSpy.count(), 1);
  QCOMPARE(appliedSpy.count(), 1);
}

void BookFilterControllerTest::languageLabel_namesAKnownCode() {
  QCOMPARE(BookFilterController::languageLabel(u"en"_s), QLocale::languageToString(QLocale::English));
}

void BookFilterControllerTest::languageLabel_unknownCode_isEchoedBack() {
  QCOMPARE(BookFilterController::languageLabel(u"zz"_s), u"zz"_s);
}

QTEST_GUILESS_MAIN(BookFilterControllerTest)
#include "BookFilterControllerTest.moc"
