#include "qmltypes/BookDTOObject.hpp"
#include "services/dto/BookDTO.hpp"

#include <QTest>

#include <utility>

using Qt::StringLiterals::operator""_s;

using readary::qmltypes::BookDTOObject;
using readary::services::BookDTO;

namespace {

BookDTO makeBook() {
  BookDTO book;
  book.isbn = 9780201616224LL;
  book.name = u"Refactoring"_s;
  book.authorName = u"Martin Fowler"_s;
  book.year = 1999;
  book.publisherName = u"Addison-Wesley"_s;
  book.description = u"Improving the design of existing code"_s;
  book.coverUrl = u"https://example.invalid/cover.jpg"_s;
  book.isHardcover = true;
  book.typeName = u"paper"_s;
  book.totalPages = 448;
  book.pagesRead = 120;
  book.globalRating = 4.5;
  book.localRating = 4.0;
  book.userRating = 9;
  book.status = 2;
  book.inWishList = true;
  book.language = u"en"_s;
  book.isCustom = true;
  book.pdfPath = u"C:/data/pdfs/1.pdf"_s;
  book.pdfSource = 2;
  book.genres = {u"Software"_s};
  return book;
}

QVariant read(const BookDTOObject &book, const char *property) {
  const QMetaObject &meta = BookDTOObject::staticMetaObject;
  const int index = meta.indexOfProperty(property);
  if (index < 0) {
    return {};
  }
  return meta.property(index).readOnGadget(&book);
}

bool write(BookDTOObject &book, const char *property, const QVariant &value) {
  const QMetaObject &meta = BookDTOObject::staticMetaObject;
  const int index = meta.indexOfProperty(property);
  if (index < 0) {
    return false;
  }
  return meta.property(index).writeOnGadget(&book, value);
}

} // namespace

// The Q_GADGET wrapper is what QML sees; the plain DTO underneath stays free of
// moc. These tests go through the metaobject, the way QML does.
class BookDTOObjectTest : public QObject {
  Q_OBJECT

private slots:
  void defaultConstructed_isAnEmptyBook();
  void constructedFromADto_copiesEveryField();
  void constructedFromAnRvalueDto_takesItsFields();
  void isABookDto_soItPassesStraightToTheTable();

  void properties_exposeQmlFriendlyNames();
  void properties_readThroughTheMetaObject();
  void properties_writeThroughTheMetaObject();
  void properties_coverEveryDtoFieldQmlNeeds();
  void unknownProperty_isNotFound();
};

void BookDTOObjectTest::defaultConstructed_isAnEmptyBook() {
  const BookDTOObject book;

  QCOMPARE(book.isbn, 0LL);
  QVERIFY(book.name.isEmpty());
  QVERIFY(book.genres.isEmpty());
}

void BookDTOObjectTest::constructedFromADto_copiesEveryField() {
  const BookDTO source = makeBook();
  const BookDTOObject book{source};

  QCOMPARE(book.isbn, source.isbn);
  QCOMPARE(book.name, source.name);
  QCOMPARE(book.authorName, source.authorName);
  QCOMPARE(book.genres, source.genres);
  QCOMPARE(book.totalPages, source.totalPages);
}

void BookDTOObjectTest::constructedFromAnRvalueDto_takesItsFields() {
  BookDTO source = makeBook();
  const BookDTOObject book{std::move(source)};

  QCOMPARE(book.isbn, 9780201616224LL);
  QCOMPARE(book.name, u"Refactoring"_s);
}

void BookDTOObjectTest::isABookDto_soItPassesStraightToTheTable() {
  // BookController hands the wrapper to BookTable::updateBook, which takes the
  // plain DTO — the inheritance is what makes that work.
  const BookDTOObject book{makeBook()};
  const BookDTO &asDto = book;

  QCOMPARE(asDto.isbn, book.isbn);
  QCOMPARE(asDto.name, book.name);
}

void BookDTOObjectTest::properties_exposeQmlFriendlyNames() {
  // QML says `book.author`, not `book.authorName`.
  const QMetaObject &meta = BookDTOObject::staticMetaObject;

  QVERIFY(meta.indexOfProperty("author") >= 0);
  QVERIFY(meta.indexOfProperty("publisher") >= 0);
  QVERIFY(meta.indexOfProperty("type") >= 0);
  QVERIFY(meta.indexOfProperty("authorName") < 0);
  QVERIFY(meta.indexOfProperty("publisherName") < 0);
  QVERIFY(meta.indexOfProperty("typeName") < 0);
}

void BookDTOObjectTest::properties_readThroughTheMetaObject() {
  const BookDTOObject book{makeBook()};

  QCOMPARE(read(book, "isbn").toLongLong(), 9780201616224LL);
  QCOMPARE(read(book, "name").toString(), u"Refactoring"_s);
  QCOMPARE(read(book, "author").toString(), u"Martin Fowler"_s);
  QCOMPARE(read(book, "publisher").toString(), u"Addison-Wesley"_s);
  QCOMPARE(read(book, "type").toString(), u"paper"_s);
  QCOMPARE(read(book, "year").toInt(), 1999);
  QCOMPARE(read(book, "totalPages").toInt(), 448);
  QCOMPARE(read(book, "pagesRead").toInt(), 120);
  QCOMPARE(read(book, "globalRating").toDouble(), 4.5);
  QCOMPARE(read(book, "localRating").toDouble(), 4.0);
  QCOMPARE(read(book, "userRating").toInt(), 9);
  QCOMPARE(read(book, "status").toInt(), 2);
  QVERIFY(read(book, "isHardcover").toBool());
  QVERIFY(read(book, "inWishList").toBool());
  QVERIFY(read(book, "isCustom").toBool());
  QCOMPARE(read(book, "pdfPath").toString(), u"C:/data/pdfs/1.pdf"_s);
  QCOMPARE(read(book, "pdfSource").toInt(), 2);
  QCOMPARE(read(book, "language").toString(), u"en"_s);
  QCOMPARE(read(book, "description").toString(), u"Improving the design of existing code"_s);
  QCOMPARE(read(book, "coverUrl").toString(), u"https://example.invalid/cover.jpg"_s);
  QCOMPARE(read(book, "genres").toStringList(), QStringList({u"Software"_s}));
}

void BookDTOObjectTest::properties_writeThroughTheMetaObject() {
  BookDTOObject book;

  QVERIFY(write(book, "name", u"Dune"_s));
  QVERIFY(write(book, "author", u"Frank Herbert"_s));
  QVERIFY(write(book, "pagesRead", 42));

  QCOMPARE(book.name, u"Dune"_s);
  QCOMPARE(book.authorName, u"Frank Herbert"_s);
  QCOMPARE(book.pagesRead, 42);
}

void BookDTOObjectTest::properties_coverEveryDtoFieldQmlNeeds() {
  const QMetaObject &meta = BookDTOObject::staticMetaObject;

  // Every DTO field except workKey, which is an import-time detail.
  QCOMPARE(meta.propertyCount(), 21);
}

void BookDTOObjectTest::unknownProperty_isNotFound() {
  QCOMPARE(BookDTOObject::staticMetaObject.indexOfProperty("workKey"), -1);
}

QTEST_GUILESS_MAIN(BookDTOObjectTest)
#include "BookDTOObjectTest.moc"
