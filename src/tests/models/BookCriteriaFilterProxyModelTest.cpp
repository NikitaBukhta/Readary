#include "models/books/BookCriteriaFilterProxyModel.hpp"
#include "models/books/BookListModelBase.hpp"
#include "models/books/GlobalBookSearchListModel.hpp"
#include "services/BookDTO.hpp"
#include "services/BookFilterCriteria.hpp"
#include "services/BookStatus.hpp"

#include <QSignalSpy>
#include <QTest>

using Qt::StringLiterals::operator""_s;

using readary::models::BookCriteriaFilterProxyModel;
using readary::models::BookListModelBase;
using readary::models::GlobalBookSearchListModel;
using readary::services::BookDTO;
using readary::services::BookFilterCriteria;
using readary::services::BookStatus;

namespace {

using Roles = BookListModelBase::Roles;

QStringList namesOf(const QAbstractItemModel &model) {
  QStringList names;
  names.reserve(model.rowCount());
  for (int row = 0; row < model.rowCount(); ++row) {
    names << model.data(model.index(row, 0), Roles::NameRole).toString();
  }
  names.sort();
  return names;
}

} // namespace

class BookCriteriaFilterProxyModelTest : public QObject {
  Q_OBJECT

private slots:
  void init();

  void withoutCriteria_everyRowPasses();
  void withoutASourceModel_thereAreNoRows();

  void languageCriterion_keepsMatchingBooks();
  void languageCriterion_matchesOneEntryOfACsvCell();
  void genreCriterion_matchesAnyOfTheBooksGenres();
  void authorCriterion_isACaseInsensitiveSubstring();
  void publisherCriterion_isACaseInsensitiveSubstring();
  void typeCriterion_keepsMatchingBooks();
  void statusCriterion_keepsMatchingBooks();
  void pageRange_honoursOpenBounds();
  void yearRange_honoursOpenBounds();
  void minRating_dropsWeakerBooks();
  void severalCriteria_areAnded();

  void setCriteria_replacesTheEarlierSet();
  void clearCriteria_restoresEveryRow();
  void clearCriteria_whenEmpty_isANoOp();
  void criteria_exposesWhatWasApplied();
  void countChanged_firesWhenTheSourceResets();

private:
  GlobalBookSearchListModel _source;
  BookCriteriaFilterProxyModel _proxy;
};

void BookCriteriaFilterProxyModelTest::init() {
  _proxy.clearCriteria();

  BookDTO dune;
  dune.isbn = 1;
  dune.name = u"Dune"_s;
  dune.authorName = u"Frank Herbert"_s;
  dune.publisherName = u"Ace Books"_s;
  dune.language = u"en"_s;
  dune.typeName = u"paper"_s;
  dune.genres = {u"Science Fiction"_s, u"Fiction"_s};
  dune.year = 1965;
  dune.totalPages = 412;
  dune.globalRating = 4.5;
  dune.status = static_cast<int>(BookStatus::InProgress);

  BookDTO refactoring;
  refactoring.isbn = 2;
  refactoring.name = u"Refactoring"_s;
  refactoring.authorName = u"Martin Fowler"_s;
  refactoring.publisherName = u"Addison-Wesley"_s;
  refactoring.language = u"en, de"_s;
  refactoring.typeName = u"ebook"_s;
  refactoring.genres = {u"Software"_s};
  refactoring.year = 1999;
  refactoring.totalPages = 448;
  refactoring.globalRating = 4.2;
  refactoring.status = static_cast<int>(BookStatus::Finished);

  BookDTO solaris;
  solaris.isbn = 3;
  solaris.name = u"Solaris"_s;
  solaris.authorName = u"Stanisław Lem"_s;
  solaris.publisherName = u"Wydawnictwo"_s;
  solaris.language = u"pl"_s;
  solaris.typeName = u"paper"_s;
  solaris.genres = {u"Science Fiction"_s};
  solaris.year = 1961;
  solaris.totalPages = 204;
  solaris.globalRating = 3.8;
  solaris.status = static_cast<int>(BookStatus::WantToRead);

  _source.setBooks({dune, refactoring, solaris});
  _proxy.setSourceModel(&_source);
}

void BookCriteriaFilterProxyModelTest::withoutCriteria_everyRowPasses() { QCOMPARE(_proxy.rowCount(), 3); }

void BookCriteriaFilterProxyModelTest::withoutASourceModel_thereAreNoRows() {
  BookCriteriaFilterProxyModel orphan;
  QCOMPARE(orphan.rowCount(), 0);
}

void BookCriteriaFilterProxyModelTest::languageCriterion_keepsMatchingBooks() {
  BookFilterCriteria criteria;
  criteria.languages = {u"pl"_s};
  _proxy.setCriteria(criteria);

  QCOMPARE(namesOf(_proxy), QStringList({u"Solaris"_s}));
}

void BookCriteriaFilterProxyModelTest::languageCriterion_matchesOneEntryOfACsvCell() {
  BookFilterCriteria criteria;
  criteria.languages = {u"de"_s};
  _proxy.setCriteria(criteria);

  QCOMPARE(namesOf(_proxy), QStringList({u"Refactoring"_s}));
}

void BookCriteriaFilterProxyModelTest::genreCriterion_matchesAnyOfTheBooksGenres() {
  BookFilterCriteria criteria;
  criteria.genres = {u"Science Fiction"_s};
  _proxy.setCriteria(criteria);

  QCOMPARE(namesOf(_proxy), QStringList({u"Dune"_s, u"Solaris"_s}));
}

void BookCriteriaFilterProxyModelTest::authorCriterion_isACaseInsensitiveSubstring() {
  BookFilterCriteria criteria;
  criteria.author = u"fowler"_s;
  _proxy.setCriteria(criteria);

  QCOMPARE(namesOf(_proxy), QStringList({u"Refactoring"_s}));
}

void BookCriteriaFilterProxyModelTest::publisherCriterion_isACaseInsensitiveSubstring() {
  BookFilterCriteria criteria;
  criteria.publisher = u"ace"_s;
  _proxy.setCriteria(criteria);

  QCOMPARE(namesOf(_proxy), QStringList({u"Dune"_s}));
}

void BookCriteriaFilterProxyModelTest::typeCriterion_keepsMatchingBooks() {
  BookFilterCriteria criteria;
  criteria.types = {u"ebook"_s};
  _proxy.setCriteria(criteria);

  QCOMPARE(namesOf(_proxy), QStringList({u"Refactoring"_s}));
}

void BookCriteriaFilterProxyModelTest::statusCriterion_keepsMatchingBooks() {
  BookFilterCriteria criteria;
  criteria.statuses = {static_cast<int>(BookStatus::Finished), static_cast<int>(BookStatus::WantToRead)};
  _proxy.setCriteria(criteria);

  QCOMPARE(namesOf(_proxy), QStringList({u"Refactoring"_s, u"Solaris"_s}));
}

void BookCriteriaFilterProxyModelTest::pageRange_honoursOpenBounds() {
  BookFilterCriteria criteria;
  criteria.minPages = 400;
  _proxy.setCriteria(criteria);
  QCOMPARE(namesOf(_proxy), QStringList({u"Dune"_s, u"Refactoring"_s}));

  criteria.minPages = 0;
  criteria.maxPages = 300;
  _proxy.setCriteria(criteria);
  QCOMPARE(namesOf(_proxy), QStringList({u"Solaris"_s}));
}

void BookCriteriaFilterProxyModelTest::yearRange_honoursOpenBounds() {
  BookFilterCriteria criteria;
  criteria.minYear = 1990;
  _proxy.setCriteria(criteria);
  QCOMPARE(namesOf(_proxy), QStringList({u"Refactoring"_s}));

  criteria.minYear = 0;
  criteria.maxYear = 1964;
  _proxy.setCriteria(criteria);
  QCOMPARE(namesOf(_proxy), QStringList({u"Solaris"_s}));
}

void BookCriteriaFilterProxyModelTest::minRating_dropsWeakerBooks() {
  BookFilterCriteria criteria;
  criteria.minRating = 4.0;
  _proxy.setCriteria(criteria);

  QCOMPARE(namesOf(_proxy), QStringList({u"Dune"_s, u"Refactoring"_s}));
}

void BookCriteriaFilterProxyModelTest::severalCriteria_areAnded() {
  BookFilterCriteria criteria;
  criteria.genres = {u"Science Fiction"_s};
  criteria.minRating = 4.0;
  _proxy.setCriteria(criteria);

  QCOMPARE(namesOf(_proxy), QStringList({u"Dune"_s}));
}

void BookCriteriaFilterProxyModelTest::setCriteria_replacesTheEarlierSet() {
  BookFilterCriteria first;
  first.languages = {u"pl"_s};
  _proxy.setCriteria(first);

  BookFilterCriteria second;
  second.types = {u"ebook"_s};
  _proxy.setCriteria(second);

  QCOMPARE(namesOf(_proxy), QStringList({u"Refactoring"_s}));
}

void BookCriteriaFilterProxyModelTest::clearCriteria_restoresEveryRow() {
  BookFilterCriteria criteria;
  criteria.languages = {u"pl"_s};
  _proxy.setCriteria(criteria);
  QCOMPARE(_proxy.rowCount(), 1);

  _proxy.clearCriteria();

  QCOMPARE(_proxy.rowCount(), 3);
}

void BookCriteriaFilterProxyModelTest::clearCriteria_whenEmpty_isANoOp() {
  _proxy.clearCriteria();
  QCOMPARE(_proxy.rowCount(), 3);
}

void BookCriteriaFilterProxyModelTest::criteria_exposesWhatWasApplied() {
  BookFilterCriteria criteria;
  criteria.author = u"Herbert"_s;
  criteria.minRating = 4.0;
  _proxy.setCriteria(criteria);

  QCOMPARE(_proxy.criteria().author, u"Herbert"_s);
  QCOMPARE(_proxy.criteria().activeCount(), 2);
}

void BookCriteriaFilterProxyModelTest::countChanged_firesWhenTheSourceResets() {
  QSignalSpy countSpy{&_proxy, &BookCriteriaFilterProxyModel::countChanged};

  _source.setBooks({});

  QVERIFY(countSpy.count() > 0);
  QCOMPARE(_proxy.rowCount(), 0);
}

QTEST_GUILESS_MAIN(BookCriteriaFilterProxyModelTest)
#include "BookCriteriaFilterProxyModelTest.moc"
