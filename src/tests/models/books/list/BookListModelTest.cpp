#include "models/books/list/BookListModel.hpp"
#include "support/TempLibrary.hpp"

#include <QSignalSpy>
#include <QTest>

using Qt::StringLiterals::operator""_s;

using readary::models::BookListModel;
using readary::tests::makeBook;
using readary::tests::TempLibrary;

namespace {

constexpr qint64 kIsbn = 9780201616224LL;
constexpr qint64 kOtherIsbn = 9781491903995LL;

} // namespace

class BookListModelTest : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void init();
  void cleanup();

  void refresh_loadsWhatTheTableHolds();
  void refresh_emitsAReset();
  void refresh_afterAnExternalInsert_picksItUp();
  void refresh_carriesGenres();
  void deleteBook_removesTheRow();
  void deleteBook_clearsAnEarlierError();
  void deleteBook_unknownIsbn_reportsAnError();
  void errorMessage_startsEmpty();
  void errorMessage_onlyChangesWhenTheTextDoes();

private:
  TempLibrary _library;
};

void BookListModelTest::initTestCase() {
  // Resources of a static lib can be stripped by the linker — see main.cpp.
  Q_INIT_RESOURCE(db_scripts);
}

void BookListModelTest::init() {
  QVERIFY(_library.open());
  QCOMPARE(_library.books()->addBook(makeBook(kIsbn, u"Refactoring"_s)), kIsbn);
}

void BookListModelTest::cleanup() { _library.close(); }

void BookListModelTest::refresh_loadsWhatTheTableHolds() {
  BookListModel model{_library.books(), nullptr};
  QCOMPARE(model.rowCount(), 0);

  model.refresh();

  QCOMPARE(model.rowCount(), 1);
  QCOMPARE(model.index(0, 0).data(BookListModel::NameRole).toString(), u"Refactoring"_s);
}

void BookListModelTest::refresh_emitsAReset() {
  BookListModel model{_library.books(), nullptr};
  QSignalSpy resetSpy{&model, &QAbstractItemModel::modelReset};

  model.refresh();

  QCOMPARE(resetSpy.count(), 1);
}

void BookListModelTest::refresh_afterAnExternalInsert_picksItUp() {
  BookListModel model{_library.books(), nullptr};
  model.refresh();
  QCOMPARE(model.rowCount(), 1);

  QCOMPARE(_library.books()->addBook(makeBook(kOtherIsbn, u"Effective Modern C++"_s)), kOtherIsbn);
  model.refresh();

  QCOMPARE(model.rowCount(), 2);
}

void BookListModelTest::refresh_carriesGenres() {
  QVERIFY(_library.books()->setGenres(kIsbn, {u"Software"_s, u"Craft"_s}));

  BookListModel model{_library.books(), nullptr};
  model.refresh();

  QCOMPARE(model.index(0, 0).data(BookListModel::GenresRole).toStringList(), QStringList({u"Craft"_s, u"Software"_s}));
}

void BookListModelTest::deleteBook_removesTheRow() {
  BookListModel model{_library.books(), nullptr};
  model.refresh();

  QVERIFY(model.deleteBook(kIsbn));

  // deleteBook refreshes itself, so the row is gone without a second call.
  QCOMPARE(model.rowCount(), 0);
  QVERIFY(model.errorMessage().isEmpty());
}

void BookListModelTest::deleteBook_clearsAnEarlierError() {
  BookListModel model{_library.books(), nullptr};
  model.refresh();

  QVERIFY(!model.deleteBook(kOtherIsbn));
  QVERIFY(!model.errorMessage().isEmpty());

  QVERIFY(model.deleteBook(kIsbn));
  QVERIFY(model.errorMessage().isEmpty());
}

void BookListModelTest::deleteBook_unknownIsbn_reportsAnError() {
  BookListModel model{_library.books(), nullptr};
  model.refresh();

  QSignalSpy errorSpy{&model, &BookListModel::errorMessageChanged};
  QVERIFY(!model.deleteBook(kOtherIsbn));

  QCOMPARE(errorSpy.count(), 1);
  QVERIFY(!model.errorMessage().isEmpty());
  QCOMPARE(model.rowCount(), 1);
}

void BookListModelTest::errorMessage_startsEmpty() {
  BookListModel model{_library.books(), nullptr};
  QVERIFY(model.errorMessage().isEmpty());
}

void BookListModelTest::errorMessage_onlyChangesWhenTheTextDoes() {
  BookListModel model{_library.books(), nullptr};
  model.refresh();

  QSignalSpy errorSpy{&model, &BookListModel::errorMessageChanged};
  QVERIFY(!model.deleteBook(kOtherIsbn));
  QVERIFY(!model.deleteBook(kOtherIsbn));

  QCOMPARE(errorSpy.count(), 1);
}

QTEST_GUILESS_MAIN(BookListModelTest)
#include "BookListModelTest.moc"
