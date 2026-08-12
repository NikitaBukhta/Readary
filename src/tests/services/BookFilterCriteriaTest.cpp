#include "services/BookFilterCriteria.hpp"

#include <QTest>

using Qt::StringLiterals::operator""_s;

using readary::services::BookDTO;
using readary::services::BookFilterCriteria;

namespace {

BookDTO makeBook() {
  BookDTO book;
  book.isbn = 9780306406157LL;
  book.name = u"Dune"_s;
  book.authorName = u"Frank Herbert"_s;
  book.publisherName = u"Ace Books"_s;
  book.year = 1965;
  book.totalPages = 412;
  book.globalRating = 4.5;
  book.language = u"en"_s;
  book.typeName = u"basic"_s;
  book.status = 2;
  book.genres = {u"Science Fiction"_s, u"Fiction"_s};
  return book;
}

} // namespace

class BookFilterCriteriaTest : public QObject {
  Q_OBJECT

private slots:
  void empty_matchesEverything();
  void language_matchesCsvCell();
  void genres_matchAnyOfTheBooksGenres();
  void author_isCaseInsensitiveSubstring();
  void publisher_isCaseInsensitiveSubstring();
  void pageRange_honoursOpenBounds();
  void yearRange_honoursOpenBounds();
  void rating_isAMinimum();
  void statusAndType_areExactMembership();
  void criteria_combineWithAnd();
  void activeCount_countsEachCriterionOnce();
  void catalogSubset_dropsLibraryOnlyTerms();
};

void BookFilterCriteriaTest::empty_matchesEverything() {
  const BookFilterCriteria criteria;
  QVERIFY(criteria.isEmpty());
  QCOMPARE(criteria.activeCount(), 0);
  QVERIFY(criteria.matches(makeBook()));
  QVERIFY(criteria.matches(BookDTO{})); // even a blank DTO
}

void BookFilterCriteriaTest::language_matchesCsvCell() {
  BookDTO book = makeBook();

  BookFilterCriteria criteria;
  criteria.languages = {u"en"_s};
  QVERIFY(criteria.matches(book));

  criteria.languages = {u"ru"_s};
  QVERIFY(!criteria.matches(book));

  // OpenLibrary joins every edition's language into one cell — an equality test
  // against the whole string would never fire.
  book.language = u"fr, en, de"_s;
  criteria.languages = {u"en"_s};
  QVERIFY(criteria.matches(book));

  criteria.languages = {u"uk"_s, u"de"_s}; // any match passes
  QVERIFY(criteria.matches(book));

  book.language.clear();
  QVERIFY(!criteria.matches(book));
}

void BookFilterCriteriaTest::genres_matchAnyOfTheBooksGenres() {
  const BookDTO book = makeBook();

  BookFilterCriteria criteria;
  criteria.genres = {u"Fiction"_s};
  QVERIFY(criteria.matches(book));

  criteria.genres = {u"science fiction"_s}; // case-insensitive
  QVERIFY(criteria.matches(book));

  criteria.genres = {u"Poetry"_s};
  QVERIFY(!criteria.matches(book));

  criteria.genres = {u"Poetry"_s, u"Fiction"_s};
  QVERIFY(criteria.matches(book));

  BookDTO noGenres = book;
  noGenres.genres.clear();
  QVERIFY(!criteria.matches(noGenres));
}

void BookFilterCriteriaTest::author_isCaseInsensitiveSubstring() {
  const BookDTO book = makeBook();

  BookFilterCriteria criteria;
  criteria.author = u"herbert"_s;
  QVERIFY(criteria.matches(book));

  // Multi-author cells like "David Thomas, Andrew Hunt" are why this is a
  // substring test rather than equality.
  BookDTO multi = book;
  multi.authorName = u"David Thomas, Andrew Hunt"_s;
  criteria.author = u"Andrew Hunt"_s;
  QVERIFY(criteria.matches(multi));

  criteria.author = u"Tolkien"_s;
  QVERIFY(!criteria.matches(book));

  criteria.author = u"   "_s; // whitespace only is not a criterion
  QVERIFY(criteria.matches(book));
  QCOMPARE(criteria.activeCount(), 0);
}

void BookFilterCriteriaTest::publisher_isCaseInsensitiveSubstring() {
  const BookDTO book = makeBook();

  BookFilterCriteria criteria;
  criteria.publisher = u"ace"_s;
  QVERIFY(criteria.matches(book));

  criteria.publisher = u"Gollancz"_s;
  QVERIFY(!criteria.matches(book));

  // OpenLibrary imports carry no publisher at all.
  BookDTO noPublisher = book;
  noPublisher.publisherName.clear();
  QVERIFY(!criteria.matches(noPublisher));
}

void BookFilterCriteriaTest::pageRange_honoursOpenBounds() {
  const BookDTO book = makeBook(); // 412 pages

  BookFilterCriteria criteria;
  criteria.minPages = 400;
  QVERIFY(criteria.matches(book));
  criteria.minPages = 500;
  QVERIFY(!criteria.matches(book));

  criteria = {};
  criteria.maxPages = 500;
  QVERIFY(criteria.matches(book));
  criteria.maxPages = 400;
  QVERIFY(!criteria.matches(book));

  criteria = {};
  criteria.minPages = 400;
  criteria.maxPages = 420;
  QVERIFY(criteria.matches(book));
  QCOMPARE(criteria.activeCount(), 1); // a range is one criterion, not two

  // Inclusive on both ends.
  criteria.minPages = 412;
  criteria.maxPages = 412;
  QVERIFY(criteria.matches(book));
}

void BookFilterCriteriaTest::yearRange_honoursOpenBounds() {
  const BookDTO book = makeBook(); // 1965

  BookFilterCriteria criteria;
  criteria.minYear = 1960;
  criteria.maxYear = 1970;
  QVERIFY(criteria.matches(book));

  criteria.maxYear = 1964;
  QVERIFY(!criteria.matches(book));

  // A book with no year survives an open lower bound but not a real one.
  BookDTO noYear = book;
  noYear.year = 0;
  criteria = {};
  criteria.maxYear = 1970;
  QVERIFY(criteria.matches(noYear));
  criteria.minYear = 1900;
  QVERIFY(!criteria.matches(noYear));
}

void BookFilterCriteriaTest::rating_isAMinimum() {
  const BookDTO book = makeBook(); // 4.5

  BookFilterCriteria criteria;
  criteria.minRating = 4.0;
  QVERIFY(criteria.matches(book));

  criteria.minRating = 4.5;
  QVERIFY(criteria.matches(book)); // inclusive

  criteria.minRating = 4.6;
  QVERIFY(!criteria.matches(book));
}

void BookFilterCriteriaTest::statusAndType_areExactMembership() {
  const BookDTO book = makeBook(); // status 2, type "basic"

  BookFilterCriteria criteria;
  criteria.statuses = {2};
  QVERIFY(criteria.matches(book));

  criteria.statuses = {1, 3};
  QVERIFY(!criteria.matches(book));

  criteria = {};
  criteria.types = {u"BASIC"_s}; // case-insensitive
  QVERIFY(criteria.matches(book));

  criteria.types = {u"delux"_s};
  QVERIFY(!criteria.matches(book));
}

void BookFilterCriteriaTest::criteria_combineWithAnd() {
  const BookDTO book = makeBook();

  BookFilterCriteria criteria;
  criteria.languages = {u"en"_s};
  criteria.author = u"Herbert"_s;
  criteria.minPages = 400;
  QVERIFY(criteria.matches(book));

  // One failing criterion rejects the book regardless of the others.
  criteria.minPages = 500;
  QVERIFY(!criteria.matches(book));
}

void BookFilterCriteriaTest::activeCount_countsEachCriterionOnce() {
  BookFilterCriteria criteria;
  QCOMPARE(criteria.activeCount(), 0);
  QVERIFY(!criteria.hasCatalogTerms());

  criteria.languages = {u"en"_s};
  criteria.genres = {u"Fiction"_s};
  criteria.author = u"Herbert"_s;
  QCOMPARE(criteria.activeCount(), 3);
  QVERIFY(criteria.hasCatalogTerms());

  // Ranges and ratings are real criteria but nothing a catalog query can carry.
  BookFilterCriteria numeric;
  numeric.minPages = 100;
  numeric.minRating = 4.0;
  QCOMPARE(numeric.activeCount(), 2);
  QVERIFY(!numeric.hasCatalogTerms());
}

void BookFilterCriteriaTest::catalogSubset_dropsLibraryOnlyTerms() {
  BookFilterCriteria criteria;
  criteria.languages = {u"en"_s};
  criteria.minPages = 100;
  criteria.statuses = {2};
  criteria.types = {u"basic"_s};

  const BookFilterCriteria subset = criteria.catalogSubset();
  QVERIFY(subset.statuses.isEmpty());
  QVERIFY(subset.types.isEmpty());
  QCOMPARE(subset.languages, criteria.languages);
  QCOMPARE(subset.minPages, criteria.minPages);

  // The point of the subset: a catalog result has no status and no edition type,
  // so the full criteria would reject every one of them.
  BookDTO online = makeBook();
  online.status = 0;
  online.typeName.clear();
  QVERIFY(!criteria.matches(online));
  QVERIFY(subset.matches(online));
}

QTEST_GUILESS_MAIN(BookFilterCriteriaTest)
#include "BookFilterCriteriaTest.moc"
