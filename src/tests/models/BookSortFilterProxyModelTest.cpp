#include "models/books/BookSortFilterProxyModel.hpp"
#include "models/books/BookListModelBase.hpp"
#include "models/books/GlobalBookSearchListModel.hpp"
#include "models/books/filters/BookFilterStrategy.hpp"
#include "services/BookDTO.hpp"
#include "services/BookStatus.hpp"

#include <QSignalSpy>
#include <QTest>

using Qt::StringLiterals::operator""_s;

using readary::models::BookListModelBase;
using readary::models::BookSortFilterProxyModel;
using readary::models::GlobalBookSearchListModel;
using readary::services::BookDTO;
using readary::services::BookStatus;

namespace filters = readary::models::filters;

namespace {

using Op = BookSortFilterProxyModel::Op;
using Roles = BookListModelBase::Roles;

BookDTO makeBook(qint64 isbn, const QString &name, int status, bool inWishList = false) {
  BookDTO book;
  book.isbn = isbn;
  book.name = name;
  book.authorName = u"Frank Herbert"_s;
  book.year = 1965;
  book.totalPages = 412;
  book.globalRating = 4.0;
  book.status = status;
  book.inWishList = inWishList;
  return book;
}

QStringList namesOf(const QAbstractItemModel &model) {
  QStringList names;
  names.reserve(model.rowCount());
  for (int row = 0; row < model.rowCount(); ++row) {
    names << model.data(model.index(row, 0), Roles::NameRole).toString();
  }
  return names;
}

} // namespace

class BookSortFilterProxyModelTest : public QObject {
  Q_OBJECT

private slots:
  void init();

  void withoutFilters_everyRowPasses();
  void withoutASourceModel_thereAreNoRows();

  void equalFilter_keepsMatchingRows();
  void notEqualFilter_dropsMatchingRows();
  void containsFilter_isCaseInsensitive();
  void containsFilter_onAMissingSubstring_dropsTheRow();
  void lessFilter_comparesNumerically();
  void lessOrEqualFilter_includesTheBound();
  void greaterFilter_comparesNumerically();
  void greaterOrEqualFilter_includesTheBound();
  void numericFilter_onNonNumericText_dropsTheRow();

  void twoFiltersOnOneRole_areOred();
  void filtersOnDifferentRoles_areAnded();
  void removeFilter_dropsOnlyThatRole();
  void removeFilter_forAnUnfilteredRole_isANoOp();
  void clearFilter_restoresEveryRow();
  void clearFilter_whenEmpty_isANoOp();

  void sorting_defaultsToNameAscending();
  void setSortField_switchesToNumericOrder();
  void setSortField_toTheSameRole_isANoOp();
  void sortDescending_reversesTheOrder();
  void setSortDescending_toTheSameOrder_isANoOp();
  void sortingRatings_comparesAsDoubles();

  void countChanged_firesWhenTheSourceResets();

  void wantToReadStrategy_selectsWantToRead();
  void wantToBuyStrategy_selectsTheWishList();
  void alreadyReadStrategy_selectsFinished();
  void readInProgressStrategy_selectsInProgress();
  void strategy_replacesAnEarlierFilter();

private:
  void seedLibrary();

  GlobalBookSearchListModel _source;
  BookSortFilterProxyModel _proxy;
};

void BookSortFilterProxyModelTest::init() {
  _proxy.clearFilter();
  _proxy.setSortField(Roles::NameRole);
  _proxy.setSortDescending(false);
  seedLibrary();
  _proxy.setSourceModel(&_source);
}

void BookSortFilterProxyModelTest::seedLibrary() {
  BookDTO dune = makeBook(1, u"Dune"_s, BookStatus::InProgress);
  dune.year = 1965;
  dune.totalPages = 412;
  dune.globalRating = 4.5;

  BookDTO refactoring = makeBook(2, u"Refactoring"_s, BookStatus::Finished);
  refactoring.authorName = u"Martin Fowler"_s;
  refactoring.year = 1999;
  refactoring.totalPages = 448;
  refactoring.globalRating = 4.2;

  BookDTO clean = makeBook(3, u"Clean Code"_s, BookStatus::WantToRead, true);
  clean.authorName = u"Robert Martin"_s;
  clean.year = 2008;
  clean.totalPages = 464;
  clean.globalRating = 3.9;

  _source.setBooks({dune, refactoring, clean});
}

void BookSortFilterProxyModelTest::withoutFilters_everyRowPasses() { QCOMPARE(_proxy.rowCount(), 3); }

void BookSortFilterProxyModelTest::withoutASourceModel_thereAreNoRows() {
  BookSortFilterProxyModel orphan;
  QCOMPARE(orphan.rowCount(), 0);
}

void BookSortFilterProxyModelTest::equalFilter_keepsMatchingRows() {
  _proxy.addFilter(Roles::StatusRole, static_cast<int>(BookStatus::Finished));
  QCOMPARE(namesOf(_proxy), QStringList({u"Refactoring"_s}));
}

void BookSortFilterProxyModelTest::notEqualFilter_dropsMatchingRows() {
  _proxy.addFilter(Roles::StatusRole, static_cast<int>(BookStatus::Finished), Op::NotEqual);
  QCOMPARE(namesOf(_proxy), QStringList({u"Clean Code"_s, u"Dune"_s}));
}

void BookSortFilterProxyModelTest::containsFilter_isCaseInsensitive() {
  _proxy.addFilter(Roles::AuthorRole, u"fowler"_s, Op::Contains);
  QCOMPARE(namesOf(_proxy), QStringList({u"Refactoring"_s}));
}

void BookSortFilterProxyModelTest::containsFilter_onAMissingSubstring_dropsTheRow() {
  _proxy.addFilter(Roles::AuthorRole, u"tolkien"_s, Op::Contains);
  QCOMPARE(_proxy.rowCount(), 0);
}

void BookSortFilterProxyModelTest::lessFilter_comparesNumerically() {
  _proxy.addFilter(Roles::YearRole, 1999, Op::Less);
  QCOMPARE(namesOf(_proxy), QStringList({u"Dune"_s}));
}

void BookSortFilterProxyModelTest::lessOrEqualFilter_includesTheBound() {
  _proxy.addFilter(Roles::YearRole, 1999, Op::LessOrEqual);
  QCOMPARE(namesOf(_proxy), QStringList({u"Dune"_s, u"Refactoring"_s}));
}

void BookSortFilterProxyModelTest::greaterFilter_comparesNumerically() {
  _proxy.addFilter(Roles::TotalPagesRole, 448, Op::Greater);
  QCOMPARE(namesOf(_proxy), QStringList({u"Clean Code"_s}));
}

void BookSortFilterProxyModelTest::greaterOrEqualFilter_includesTheBound() {
  _proxy.addFilter(Roles::TotalPagesRole, 448, Op::GreaterOrEqual);
  QCOMPARE(namesOf(_proxy), QStringList({u"Clean Code"_s, u"Refactoring"_s}));
}

void BookSortFilterProxyModelTest::numericFilter_onNonNumericText_dropsTheRow() {
  // A comparison that cannot be made numerically is a non-match, not a pass.
  _proxy.addFilter(Roles::NameRole, 100, Op::Greater);
  QCOMPARE(_proxy.rowCount(), 0);
}

void BookSortFilterProxyModelTest::twoFiltersOnOneRole_areOred() {
  _proxy.addFilter(Roles::StatusRole, static_cast<int>(BookStatus::Finished));
  _proxy.addFilter(Roles::StatusRole, static_cast<int>(BookStatus::InProgress));

  QCOMPARE(namesOf(_proxy), QStringList({u"Dune"_s, u"Refactoring"_s}));
}

void BookSortFilterProxyModelTest::filtersOnDifferentRoles_areAnded() {
  _proxy.addFilter(Roles::StatusRole, static_cast<int>(BookStatus::Finished));
  _proxy.addFilter(Roles::YearRole, 2000, Op::Greater);

  QCOMPARE(_proxy.rowCount(), 0);
}

void BookSortFilterProxyModelTest::removeFilter_dropsOnlyThatRole() {
  _proxy.addFilter(Roles::StatusRole, static_cast<int>(BookStatus::Finished));
  _proxy.addFilter(Roles::YearRole, 2000, Op::Greater);

  _proxy.removeFilter(Roles::YearRole);

  QCOMPARE(namesOf(_proxy), QStringList({u"Refactoring"_s}));
}

void BookSortFilterProxyModelTest::removeFilter_forAnUnfilteredRole_isANoOp() {
  _proxy.addFilter(Roles::StatusRole, static_cast<int>(BookStatus::Finished));

  _proxy.removeFilter(Roles::AuthorRole);

  QCOMPARE(namesOf(_proxy), QStringList({u"Refactoring"_s}));
}

void BookSortFilterProxyModelTest::clearFilter_restoresEveryRow() {
  _proxy.addFilter(Roles::StatusRole, static_cast<int>(BookStatus::Finished));
  QCOMPARE(_proxy.rowCount(), 1);

  _proxy.clearFilter();

  QCOMPARE(_proxy.rowCount(), 3);
}

void BookSortFilterProxyModelTest::clearFilter_whenEmpty_isANoOp() {
  _proxy.clearFilter();
  QCOMPARE(_proxy.rowCount(), 3);
}

void BookSortFilterProxyModelTest::sorting_defaultsToNameAscending() {
  QCOMPARE(namesOf(_proxy), QStringList({u"Clean Code"_s, u"Dune"_s, u"Refactoring"_s}));
}

void BookSortFilterProxyModelTest::setSortField_switchesToNumericOrder() {
  _proxy.setSortField(Roles::YearRole);
  QCOMPARE(namesOf(_proxy), QStringList({u"Dune"_s, u"Refactoring"_s, u"Clean Code"_s}));
}

void BookSortFilterProxyModelTest::setSortField_toTheSameRole_isANoOp() {
  QSignalSpy fieldSpy{&_proxy, &BookSortFilterProxyModel::sortFieldChanged};

  _proxy.setSortField(Roles::NameRole);

  QCOMPARE(fieldSpy.count(), 0);
}

void BookSortFilterProxyModelTest::sortDescending_reversesTheOrder() {
  QSignalSpy orderSpy{&_proxy, &BookSortFilterProxyModel::sortDescendingChanged};

  _proxy.setSortDescending(true);

  QVERIFY(_proxy.sortDescending());
  QCOMPARE(orderSpy.count(), 1);
  QCOMPARE(namesOf(_proxy), QStringList({u"Refactoring"_s, u"Dune"_s, u"Clean Code"_s}));
}

void BookSortFilterProxyModelTest::setSortDescending_toTheSameOrder_isANoOp() {
  QSignalSpy orderSpy{&_proxy, &BookSortFilterProxyModel::sortDescendingChanged};

  _proxy.setSortDescending(false);

  QCOMPARE(orderSpy.count(), 0);
}

void BookSortFilterProxyModelTest::sortingRatings_comparesAsDoubles() {
  _proxy.setSortField(Roles::GlobalRatingRole);
  QCOMPARE(namesOf(_proxy), QStringList({u"Clean Code"_s, u"Refactoring"_s, u"Dune"_s}));
}

void BookSortFilterProxyModelTest::countChanged_firesWhenTheSourceResets() {
  QSignalSpy countSpy{&_proxy, &BookSortFilterProxyModel::countChanged};

  _source.setBooks({makeBook(4, u"Dune Messiah"_s, BookStatus::None)});

  QVERIFY(countSpy.count() > 0);
  QCOMPARE(_proxy.rowCount(), 1);
}

void BookSortFilterProxyModelTest::wantToReadStrategy_selectsWantToRead() {
  filters::WantToReadFilterStrategy{}.apply(&_proxy);
  QCOMPARE(namesOf(_proxy), QStringList({u"Clean Code"_s}));
}

void BookSortFilterProxyModelTest::wantToBuyStrategy_selectsTheWishList() {
  filters::WantToBuyFilterStrategy{}.apply(&_proxy);
  QCOMPARE(namesOf(_proxy), QStringList({u"Clean Code"_s}));
}

void BookSortFilterProxyModelTest::alreadyReadStrategy_selectsFinished() {
  filters::AlreadyReadFilterStrategy{}.apply(&_proxy);
  QCOMPARE(namesOf(_proxy), QStringList({u"Refactoring"_s}));
}

void BookSortFilterProxyModelTest::readInProgressStrategy_selectsInProgress() {
  filters::ReadInProgressFilterStrategy{}.apply(&_proxy);
  QCOMPARE(namesOf(_proxy), QStringList({u"Dune"_s}));
}

void BookSortFilterProxyModelTest::strategy_replacesAnEarlierFilter() {
  // Every strategy clears first, so switching category never intersects the two.
  filters::AlreadyReadFilterStrategy{}.apply(&_proxy);
  filters::ReadInProgressFilterStrategy{}.apply(&_proxy);

  QCOMPARE(namesOf(_proxy), QStringList({u"Dune"_s}));
}

QTEST_GUILESS_MAIN(BookSortFilterProxyModelTest)
#include "BookSortFilterProxyModelTest.moc"
