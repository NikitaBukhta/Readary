#include "models/books/BookListModelBase.hpp"
#include "models/books/GlobalBookSearchListModel.hpp"
#include "services/BookDTO.hpp"

#include <QSignalSpy>
#include <QTest>

using Qt::StringLiterals::operator""_s;

using readary::models::BookListModelBase;
using readary::models::GlobalBookSearchListModel;
using readary::services::BookDTO;

namespace {

using Roles = BookListModelBase::Roles;

BookDTO makeBook(qint64 isbn, const QString &name) {
  BookDTO book;
  book.isbn = isbn;
  book.name = name;
  book.authorName = u"Frank Herbert"_s;
  book.year = 1965;
  book.publisherName = u"Ace Books"_s;
  book.description = u"Arrakis"_s;
  book.coverUrl = u"https://example.invalid/dune.jpg"_s;
  book.isHardcover = true;
  book.typeName = u"paper"_s;
  book.totalPages = 412;
  book.pagesRead = 100;
  book.globalRating = 4.5;
  book.localRating = 4.2;
  book.userRating = 9;
  book.status = 2;
  book.inWishList = true;
  book.language = u"en"_s;
  book.genres = {u"Science Fiction"_s};
  return book;
}

} // namespace

// GlobalBookSearchListModel is the thinnest concrete BookListModelBase, so it
// stands in for the base class the four book models share.
class BookListModelBaseTest : public QObject {
  Q_OBJECT

private slots:
  void newModel_isEmpty();
  void setBooks_replacesEveryRow();
  void setBooks_emitsAReset();
  void rowCount_underAValidParent_isZero();
  void data_exposesEveryRole();
  void data_outOfRangeIndex_isInvalid();
  void data_invalidIndex_isInvalid();
  void data_unknownRole_isInvalid();
  void roleNames_coverEveryRole();
  void appendBooks_addsToTheEnd();
  void appendBooks_emitsRowsInsertedForTheNewRange();
  void appendBooks_withNothing_isANoOp();
  void contains_findsAStoredIsbn();
  void contains_missingIsbn_isFalse();
  void getBook_returnsTheStoredBook();
  void getBook_missingIsbn_returnsADefaultBook();
  void books_exposeTheBackingList();
  void refresh_clearsTheSearchResults();
};

void BookListModelBaseTest::newModel_isEmpty() {
  GlobalBookSearchListModel model;
  QCOMPARE(model.rowCount(), 0);
  QVERIFY(model.books().isEmpty());
}

void BookListModelBaseTest::setBooks_replacesEveryRow() {
  GlobalBookSearchListModel model;
  model.setBooks({makeBook(1, u"Dune"_s), makeBook(2, u"Messiah"_s)});
  QCOMPARE(model.rowCount(), 2);

  model.setBooks({makeBook(3, u"Children"_s)});
  QCOMPARE(model.rowCount(), 1);
  QCOMPARE(model.index(0, 0).data(Roles::NameRole).toString(), u"Children"_s);
}

void BookListModelBaseTest::setBooks_emitsAReset() {
  GlobalBookSearchListModel model;
  QSignalSpy resetSpy{&model, &QAbstractItemModel::modelReset};

  model.setBooks({makeBook(1, u"Dune"_s)});

  QCOMPARE(resetSpy.count(), 1);
}

void BookListModelBaseTest::rowCount_underAValidParent_isZero() {
  GlobalBookSearchListModel model;
  model.setBooks({makeBook(1, u"Dune"_s)});

  QCOMPARE(model.rowCount(model.index(0, 0)), 0);
}

void BookListModelBaseTest::data_exposesEveryRole() {
  GlobalBookSearchListModel model;
  const BookDTO book = makeBook(9780441013593LL, u"Dune"_s);
  model.setBooks({book});

  const QModelIndex idx = model.index(0, 0);
  QCOMPARE(idx.data(Roles::IsbnRole).toLongLong(), book.isbn);
  QCOMPARE(idx.data(Roles::NameRole).toString(), book.name);
  QCOMPARE(idx.data(Roles::AuthorRole).toString(), book.authorName);
  QCOMPARE(idx.data(Roles::YearRole).toInt(), book.year);
  QCOMPARE(idx.data(Roles::PublisherRole).toString(), book.publisherName);
  QCOMPARE(idx.data(Roles::DescriptionRole).toString(), book.description);
  QCOMPARE(idx.data(Roles::CoverUrlRole).toString(), book.coverUrl);
  QCOMPARE(idx.data(Roles::IsHardcoverRole).toBool(), book.isHardcover);
  QCOMPARE(idx.data(Roles::TypeRole).toString(), book.typeName);
  QCOMPARE(idx.data(Roles::TotalPagesRole).toInt(), book.totalPages);
  QCOMPARE(idx.data(Roles::PagesReadRole).toInt(), book.pagesRead);
  QCOMPARE(idx.data(Roles::GlobalRatingRole).toDouble(), book.globalRating);
  QCOMPARE(idx.data(Roles::LocalRatingRole).toDouble(), book.localRating);
  QCOMPARE(idx.data(Roles::UserRatingRole).toInt(), book.userRating);
  QCOMPARE(idx.data(Roles::StatusRole).toInt(), book.status);
  QCOMPARE(idx.data(Roles::InWishListRole).toBool(), book.inWishList);
  QCOMPARE(idx.data(Roles::LanguageRole).toString(), book.language);
  QCOMPARE(idx.data(Roles::GenresRole).toStringList(), book.genres);
}

void BookListModelBaseTest::data_outOfRangeIndex_isInvalid() {
  GlobalBookSearchListModel model;
  model.setBooks({makeBook(1, u"Dune"_s)});

  QVERIFY(!model.data(model.index(5, 0), Roles::NameRole).isValid());
}

void BookListModelBaseTest::data_invalidIndex_isInvalid() {
  GlobalBookSearchListModel model;
  model.setBooks({makeBook(1, u"Dune"_s)});

  QVERIFY(!model.data(QModelIndex{}, Roles::NameRole).isValid());
}

void BookListModelBaseTest::data_unknownRole_isInvalid() {
  GlobalBookSearchListModel model;
  model.setBooks({makeBook(1, u"Dune"_s)});

  QVERIFY(!model.index(0, 0).data(Qt::DisplayRole).isValid());
}

void BookListModelBaseTest::roleNames_coverEveryRole() {
  GlobalBookSearchListModel model;
  const QHash<int, QByteArray> names = model.roleNames();

  QCOMPARE(names.size(), Roles::GenresRole - Roles::IsbnRole + 1);
  QCOMPARE(names.value(Roles::IsbnRole), QByteArray{"isbn"});
  QCOMPARE(names.value(Roles::AuthorRole), QByteArray{"author"});
  QCOMPARE(names.value(Roles::PublisherRole), QByteArray{"publisher"});
  QCOMPARE(names.value(Roles::TypeRole), QByteArray{"type"});
  QCOMPARE(names.value(Roles::GenresRole), QByteArray{"genres"});
}

void BookListModelBaseTest::appendBooks_addsToTheEnd() {
  GlobalBookSearchListModel model;
  model.setBooks({makeBook(1, u"Dune"_s)});

  model.appendBooks({makeBook(2, u"Messiah"_s), makeBook(3, u"Children"_s)});

  QCOMPARE(model.rowCount(), 3);
  QCOMPARE(model.index(0, 0).data(Roles::NameRole).toString(), u"Dune"_s);
  QCOMPARE(model.index(2, 0).data(Roles::NameRole).toString(), u"Children"_s);
}

void BookListModelBaseTest::appendBooks_emitsRowsInsertedForTheNewRange() {
  GlobalBookSearchListModel model;
  model.setBooks({makeBook(1, u"Dune"_s)});

  QSignalSpy insertedSpy{&model, &QAbstractItemModel::rowsInserted};
  model.appendBooks({makeBook(2, u"Messiah"_s), makeBook(3, u"Children"_s)});

  QCOMPARE(insertedSpy.count(), 1);
  QCOMPARE(insertedSpy.first().at(1).toInt(), 1);
  QCOMPARE(insertedSpy.first().at(2).toInt(), 2);
}

void BookListModelBaseTest::appendBooks_withNothing_isANoOp() {
  GlobalBookSearchListModel model;
  model.setBooks({makeBook(1, u"Dune"_s)});

  QSignalSpy insertedSpy{&model, &QAbstractItemModel::rowsInserted};
  model.appendBooks({});

  QCOMPARE(insertedSpy.count(), 0);
  QCOMPARE(model.rowCount(), 1);
}

void BookListModelBaseTest::contains_findsAStoredIsbn() {
  GlobalBookSearchListModel model;
  model.setBooks({makeBook(9780441013593LL, u"Dune"_s)});

  QVERIFY(model.contains(9780441013593LL));
}

void BookListModelBaseTest::contains_missingIsbn_isFalse() {
  GlobalBookSearchListModel model;
  model.setBooks({makeBook(1, u"Dune"_s)});

  QVERIFY(!model.contains(2));
}

void BookListModelBaseTest::getBook_returnsTheStoredBook() {
  GlobalBookSearchListModel model;
  model.setBooks({makeBook(1, u"Dune"_s), makeBook(2, u"Messiah"_s)});

  QCOMPARE(model.getBook(2).name, u"Messiah"_s);
}

void BookListModelBaseTest::getBook_missingIsbn_returnsADefaultBook() {
  GlobalBookSearchListModel model;
  model.setBooks({makeBook(1, u"Dune"_s)});

  const BookDTO missing = model.getBook(404);
  QCOMPARE(missing.isbn, 0LL);
  QVERIFY(missing.name.isEmpty());
}

void BookListModelBaseTest::books_exposeTheBackingList() {
  GlobalBookSearchListModel model;
  model.setBooks({makeBook(1, u"Dune"_s), makeBook(2, u"Messiah"_s)});

  QCOMPARE(model.books().size(), 2);
  QCOMPARE(model.books().first().name, u"Dune"_s);
}

void BookListModelBaseTest::refresh_clearsTheSearchResults() {
  // Online results have no source to re-read, so refreshing empties the model.
  GlobalBookSearchListModel model;
  model.setBooks({makeBook(1, u"Dune"_s)});

  model.refresh();

  QCOMPARE(model.rowCount(), 0);
}

QTEST_GUILESS_MAIN(BookListModelBaseTest)
#include "BookListModelBaseTest.moc"
