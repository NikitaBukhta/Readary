#include "services/BookDTO.hpp"

#include <QTest>

using Qt::StringLiterals::operator""_s;

using readary::services::BookDTO;

namespace {

BookDTO makeFullBook() {
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
  book.workKey = u"/works/OL1W"_s;
  book.genres = {u"Software"_s, u"Programming"_s};
  return book;
}

} // namespace

class BookDTOTest : public QObject {
  Q_OBJECT

private slots:
  void fromMap_readsTheSqlColumnNames();
  void fromMap_missingKeys_leaveDefaults();
  void fromMap_emptyMap_isAnEmptyBook();
  void fromMap_coercesStringsFromTheDriver();
  void fromMap_genres_acceptAStringList();
  void toMap_usesTheSqlColumnNames();
  void toMap_carriesEveryField();
  void roundTrip_throughAMap_preservesEveryField();
};

void BookDTOTest::fromMap_readsTheSqlColumnNames() {
  // The map keys are SQL column names, which differ from the member names for
  // author, publisher and type.
  const QVariantMap row{
      {u"isbn"_s, 9780201616224LL},          {u"name"_s, u"Refactoring"_s}, {u"author"_s, u"Martin Fowler"_s},
      {u"publisher"_s, u"Addison-Wesley"_s}, {u"type"_s, u"paper"_s},
  };

  const BookDTO book = BookDTO::fromMap(row);
  QCOMPARE(book.isbn, 9780201616224LL);
  QCOMPARE(book.name, u"Refactoring"_s);
  QCOMPARE(book.authorName, u"Martin Fowler"_s);
  QCOMPARE(book.publisherName, u"Addison-Wesley"_s);
  QCOMPARE(book.typeName, u"paper"_s);
}

void BookDTOTest::fromMap_missingKeys_leaveDefaults() {
  const BookDTO book = BookDTO::fromMap({{u"isbn"_s, 1LL}});

  QCOMPARE(book.isbn, 1LL);
  QVERIFY(book.name.isEmpty());
  QCOMPARE(book.year, 0);
  QCOMPARE(book.totalPages, 0);
  QCOMPARE(book.pagesRead, 0);
  QCOMPARE(book.status, 0);
  QVERIFY(!book.isHardcover);
  QVERIFY(!book.inWishList);
  QVERIFY(!book.isCustom);
  QVERIFY(book.pdfPath.isEmpty());
  QCOMPARE(book.pdfSource, 0);
  QVERIFY(book.genres.isEmpty());
}

void BookDTOTest::fromMap_emptyMap_isAnEmptyBook() {
  const BookDTO book = BookDTO::fromMap({});
  QCOMPARE(book.isbn, 0LL);
  QCOMPARE(book.globalRating, 0.0);
}

void BookDTOTest::fromMap_coercesStringsFromTheDriver() {
  // SQLite can hand numbers back as text; the DTO converts rather than zeroing.
  const QVariantMap row{
      {u"isbn"_s, u"9780201616224"_s},
      {u"totalPages"_s, u"448"_s},
      {u"globalRating"_s, u"4.5"_s},
      {u"isHardcover"_s, 1},
  };

  const BookDTO book = BookDTO::fromMap(row);
  QCOMPARE(book.isbn, 9780201616224LL);
  QCOMPARE(book.totalPages, 448);
  QCOMPARE(book.globalRating, 4.5);
  QVERIFY(book.isHardcover);
}

void BookDTOTest::fromMap_genres_acceptAStringList() {
  const BookDTO book = BookDTO::fromMap({{u"genres"_s, QStringList({u"Software"_s, u"Programming"_s})}});
  QCOMPARE(book.genres, QStringList({u"Software"_s, u"Programming"_s}));
}

void BookDTOTest::toMap_usesTheSqlColumnNames() {
  const QVariantMap row = makeFullBook().toMap();

  QVERIFY(row.contains(u"author"_s));
  QVERIFY(row.contains(u"publisher"_s));
  QVERIFY(row.contains(u"type"_s));
  QVERIFY(!row.contains(u"authorName"_s));
  QVERIFY(!row.contains(u"publisherName"_s));
  QVERIFY(!row.contains(u"typeName"_s));
}

void BookDTOTest::toMap_carriesEveryField() {
  const BookDTO book = makeFullBook();
  const QVariantMap row = book.toMap();

  QCOMPARE(row.value(u"isbn"_s).toLongLong(), book.isbn);
  QCOMPARE(row.value(u"name"_s).toString(), book.name);
  QCOMPARE(row.value(u"author"_s).toString(), book.authorName);
  QCOMPARE(row.value(u"year"_s).toInt(), book.year);
  QCOMPARE(row.value(u"publisher"_s).toString(), book.publisherName);
  QCOMPARE(row.value(u"description"_s).toString(), book.description);
  QCOMPARE(row.value(u"coverUrl"_s).toString(), book.coverUrl);
  QCOMPARE(row.value(u"isHardcover"_s).toBool(), book.isHardcover);
  QCOMPARE(row.value(u"type"_s).toString(), book.typeName);
  QCOMPARE(row.value(u"totalPages"_s).toInt(), book.totalPages);
  QCOMPARE(row.value(u"pagesRead"_s).toInt(), book.pagesRead);
  QCOMPARE(row.value(u"globalRating"_s).toDouble(), book.globalRating);
  QCOMPARE(row.value(u"localRating"_s).toDouble(), book.localRating);
  QCOMPARE(row.value(u"userRating"_s).toInt(), book.userRating);
  QCOMPARE(row.value(u"status"_s).toInt(), book.status);
  QCOMPARE(row.value(u"inWishList"_s).toBool(), book.inWishList);
  QCOMPARE(row.value(u"language"_s).toString(), book.language);
  QCOMPARE(row.value(u"isCustom"_s).toBool(), book.isCustom);
  QCOMPARE(row.value(u"pdfPath"_s).toString(), book.pdfPath);
  QCOMPARE(row.value(u"pdfSource"_s).toInt(), book.pdfSource);
  QCOMPARE(row.value(u"workKey"_s).toString(), book.workKey);
  QCOMPARE(row.value(u"genres"_s).toStringList(), book.genres);
}

void BookDTOTest::roundTrip_throughAMap_preservesEveryField() {
  const BookDTO original = makeFullBook();
  const BookDTO restored = BookDTO::fromMap(original.toMap());

  QCOMPARE(restored.isbn, original.isbn);
  QCOMPARE(restored.name, original.name);
  QCOMPARE(restored.authorName, original.authorName);
  QCOMPARE(restored.year, original.year);
  QCOMPARE(restored.publisherName, original.publisherName);
  QCOMPARE(restored.description, original.description);
  QCOMPARE(restored.coverUrl, original.coverUrl);
  QCOMPARE(restored.isHardcover, original.isHardcover);
  QCOMPARE(restored.typeName, original.typeName);
  QCOMPARE(restored.totalPages, original.totalPages);
  QCOMPARE(restored.pagesRead, original.pagesRead);
  QCOMPARE(restored.globalRating, original.globalRating);
  QCOMPARE(restored.localRating, original.localRating);
  QCOMPARE(restored.userRating, original.userRating);
  QCOMPARE(restored.status, original.status);
  QCOMPARE(restored.inWishList, original.inWishList);
  QCOMPARE(restored.language, original.language);
  QCOMPARE(restored.isCustom, original.isCustom);
  QCOMPARE(restored.pdfPath, original.pdfPath);
  QCOMPARE(restored.pdfSource, original.pdfSource);
  QCOMPARE(restored.genres, original.genres);
  QCOMPARE(restored.workKey, original.workKey);
}

QTEST_GUILESS_MAIN(BookDTOTest)
#include "BookDTOTest.moc"
