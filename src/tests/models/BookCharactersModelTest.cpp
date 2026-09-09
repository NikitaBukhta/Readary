#include "models/books/BookCharactersModel.hpp"
#include "support/TempLibrary.hpp"

#include <QSignalSpy>
#include <QTest>

using Qt::StringLiterals::operator""_s;

using readary::models::BookCharactersModel;
using readary::tests::makeBook;
using readary::tests::TempLibrary;

namespace {

constexpr qint64 kIsbn = 9780201616224LL;
constexpr qint64 kEmptyIsbn = 9781491903995LL;

// Seven characters: one page of five, then a short second page.
constexpr auto kSeedCharacters = R"(
INSERT INTO book_characters (book_isbn, name, role) VALUES
  (9780201616224, 'Extract Method',     'Composing Methods'),
  (9780201616224, 'Inline Method',      'Composing Methods'),
  (9780201616224, 'Move Method',        'Moving Features'),
  (9780201616224, 'Extract Class',      'Moving Features'),
  (9780201616224, 'Rename Method',      'Making Calls Simpler'),
  (9780201616224, 'Replace Temp',       NULL),
  (9780201616224, 'Introduce Explain',  NULL)
)";

} // namespace

class BookCharactersModelTest : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void init();
  void cleanup();

  void newModel_isEmpty();
  void setBookIsbn_showsTheFirstPage();
  void setBookIsbn_emitsAReset();
  void setBookIsbn_zero_clearsTheModel();
  void setBookIsbn_unknownBook_hasNoRows();
  void setBookIsbn_reload_collapsesBackToOnePage();
  void rows_exposeEveryRole();
  void rows_nullRole_isEmpty();
  void data_outOfRangeIndex_isInvalid();
  void roleNames_coverEveryRole();
  void loadMore_revealsTheRest();
  void loadMore_atTheEnd_isANoOp();
  void loadMore_emitsRowsInserted();
  void hide_collapsesToTheFirstPage();
  void hide_whenCollapsed_isANoOp();
  void hide_emitsRowsRemoved();
  void shortJournal_neverOffersPaging();

private:
  TempLibrary _library;
};

void BookCharactersModelTest::initTestCase() {
  // Resources of a static lib can be stripped by the linker — see main.cpp.
  Q_INIT_RESOURCE(db_scripts);
}

void BookCharactersModelTest::init() {
  QVERIFY(_library.open());
  QCOMPARE(_library.books()->addBook(makeBook(kIsbn, u"Refactoring"_s)), kIsbn);
  QCOMPARE(_library.books()->addBook(makeBook(kEmptyIsbn, u"Effective Modern C++"_s)), kEmptyIsbn);
  QVERIFY(_library.db()->exec(QString::fromUtf8(kSeedCharacters)));
}

void BookCharactersModelTest::cleanup() { _library.close(); }

void BookCharactersModelTest::newModel_isEmpty() {
  BookCharactersModel model{_library.books()};
  QCOMPARE(model.rowCount(), 0);
  QVERIFY(!model.canLoadMore());
  QVERIFY(!model.canHide());
}

void BookCharactersModelTest::setBookIsbn_showsTheFirstPage() {
  BookCharactersModel model{_library.books()};
  model.setBookIsbn(kIsbn);

  QCOMPARE(model.rowCount(), 5);
  QVERIFY(model.canLoadMore());
  QVERIFY(!model.canHide());
}

void BookCharactersModelTest::setBookIsbn_emitsAReset() {
  BookCharactersModel model{_library.books()};
  QSignalSpy resetSpy{&model, &QAbstractItemModel::modelReset};

  model.setBookIsbn(kIsbn);

  QCOMPARE(resetSpy.count(), 1);
}

void BookCharactersModelTest::setBookIsbn_zero_clearsTheModel() {
  BookCharactersModel model{_library.books()};
  model.setBookIsbn(kIsbn);

  model.setBookIsbn(0);

  QCOMPARE(model.rowCount(), 0);
  QVERIFY(!model.canLoadMore());
}

void BookCharactersModelTest::setBookIsbn_unknownBook_hasNoRows() {
  BookCharactersModel model{_library.books()};
  model.setBookIsbn(kEmptyIsbn);

  QCOMPARE(model.rowCount(), 0);
  QVERIFY(!model.canLoadMore());
  QVERIFY(!model.canHide());
}

void BookCharactersModelTest::setBookIsbn_reload_collapsesBackToOnePage() {
  // Unlike the reading history, the characters list always reopens collapsed.
  BookCharactersModel model{_library.books()};
  model.setBookIsbn(kIsbn);
  model.loadMore();
  QCOMPARE(model.rowCount(), 7);

  model.setBookIsbn(kIsbn);

  QCOMPARE(model.rowCount(), 5);
}

void BookCharactersModelTest::rows_exposeEveryRole() {
  BookCharactersModel model{_library.books()};
  model.setBookIsbn(kIsbn);

  const QModelIndex first = model.index(0, 0);
  QVERIFY(first.data(BookCharactersModel::IdRole).toLongLong() > 0);
  QCOMPARE(first.data(BookCharactersModel::NameRole).toString(), u"Extract Method"_s);
  QCOMPARE(first.data(BookCharactersModel::RoleRole).toString(), u"Composing Methods"_s);
}

void BookCharactersModelTest::rows_nullRole_isEmpty() {
  BookCharactersModel model{_library.books()};
  model.setBookIsbn(kIsbn);
  model.loadMore();

  QVERIFY(model.index(5, 0).data(BookCharactersModel::RoleRole).toString().isEmpty());
}

void BookCharactersModelTest::data_outOfRangeIndex_isInvalid() {
  BookCharactersModel model{_library.books()};
  model.setBookIsbn(kIsbn);

  // Row 6 exists in the journal but is outside the visible page.
  QVERIFY(!model.data(model.index(6, 0), BookCharactersModel::NameRole).isValid());
  QVERIFY(!model.data(QModelIndex{}, BookCharactersModel::NameRole).isValid());
  QVERIFY(!model.index(0, 0).data(Qt::DisplayRole).isValid());
}

void BookCharactersModelTest::roleNames_coverEveryRole() {
  BookCharactersModel model{_library.books()};
  const QHash<int, QByteArray> names = model.roleNames();

  QCOMPARE(names.size(), 3);
  QCOMPARE(names.value(BookCharactersModel::IdRole), QByteArray{"id"});
  QCOMPARE(names.value(BookCharactersModel::NameRole), QByteArray{"name"});
  QCOMPARE(names.value(BookCharactersModel::RoleRole), QByteArray{"role"});
}

void BookCharactersModelTest::loadMore_revealsTheRest() {
  BookCharactersModel model{_library.books()};
  model.setBookIsbn(kIsbn);

  model.loadMore();

  QCOMPARE(model.rowCount(), 7);
  QVERIFY(!model.canLoadMore());
  QVERIFY(model.canHide());
}

void BookCharactersModelTest::loadMore_atTheEnd_isANoOp() {
  BookCharactersModel model{_library.books()};
  model.setBookIsbn(kIsbn);
  model.loadMore();

  QSignalSpy insertedSpy{&model, &QAbstractItemModel::rowsInserted};
  model.loadMore();

  QCOMPARE(insertedSpy.count(), 0);
  QCOMPARE(model.rowCount(), 7);
}

void BookCharactersModelTest::loadMore_emitsRowsInserted() {
  BookCharactersModel model{_library.books()};
  model.setBookIsbn(kIsbn);

  QSignalSpy insertedSpy{&model, &QAbstractItemModel::rowsInserted};
  model.loadMore();

  QCOMPARE(insertedSpy.count(), 1);
  QCOMPARE(insertedSpy.first().at(1).toInt(), 5);
  QCOMPARE(insertedSpy.first().at(2).toInt(), 6);
}

void BookCharactersModelTest::hide_collapsesToTheFirstPage() {
  BookCharactersModel model{_library.books()};
  model.setBookIsbn(kIsbn);
  model.loadMore();

  model.hide();

  QCOMPARE(model.rowCount(), 5);
  QVERIFY(model.canLoadMore());
  QVERIFY(!model.canHide());
}

void BookCharactersModelTest::hide_whenCollapsed_isANoOp() {
  BookCharactersModel model{_library.books()};
  model.setBookIsbn(kIsbn);

  QSignalSpy removedSpy{&model, &QAbstractItemModel::rowsRemoved};
  model.hide();

  QCOMPARE(removedSpy.count(), 0);
  QCOMPARE(model.rowCount(), 5);
}

void BookCharactersModelTest::hide_emitsRowsRemoved() {
  BookCharactersModel model{_library.books()};
  model.setBookIsbn(kIsbn);
  model.loadMore();

  QSignalSpy removedSpy{&model, &QAbstractItemModel::rowsRemoved};
  model.hide();

  QCOMPARE(removedSpy.count(), 1);
  QCOMPARE(removedSpy.first().at(1).toInt(), 5);
  QCOMPARE(removedSpy.first().at(2).toInt(), 6);
}

void BookCharactersModelTest::shortJournal_neverOffersPaging() {
  QVERIFY(_library.db()->exec(u"DELETE FROM book_characters WHERE name NOT IN ('Extract Method', 'Inline Method')"_s));

  BookCharactersModel model{_library.books()};
  model.setBookIsbn(kIsbn);

  QCOMPARE(model.rowCount(), 2);
  QVERIFY(!model.canLoadMore());
  QVERIFY(!model.canHide());
}

QTEST_GUILESS_MAIN(BookCharactersModelTest)
#include "BookCharactersModelTest.moc"
