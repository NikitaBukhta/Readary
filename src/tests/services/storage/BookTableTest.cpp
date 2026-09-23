#include "services/storage/BookTable.hpp"
#include "core/db/SqlQueryBuilder.hpp"
#include "services/dto/BookStatus.hpp"
#include "support/TempLibrary.hpp"

#include <QDateTime>
#include <QTest>

using Qt::StringLiterals::operator""_s;

using readary::core::SqlQueryBuilder;
using readary::services::BookDTO;
using readary::services::BookStatus;
using readary::tests::makeBook;
using readary::tests::TempLibrary;

namespace {

constexpr qint64 kIsbn = 9780201616224LL;
constexpr qint64 kOtherIsbn = 9781491903995LL;

} // namespace

class BookTableTest : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void init();
  void cleanup();

  void addBook_storesEveryField();
  void addBook_returnsTheIsbnItWasGiven();
  void addBook_duplicateIsbn_isRejected();
  void addBook_withGenres_linksThem();
  void addBook_violatingASchemaCheck_isRejected();
  void addBook_withoutAPageCount_storesNull();
  void addBook_keepsTheCustomFlag();
  void addBook_keepsThePdfColumns();
  void updateBook_writesThePdfColumns();

  void getAllBooks_onAnEmptyLibrary_returnsNothing();
  void getAllBooks_attachesGenresToTheRightBook();

  void updateBook_writesTheNewValues();
  void updateBook_unknownIsbn_returnsFalse();
  void updateBook_withGenres_replacesTheOldSet();
  void updateBook_withoutGenres_leavesTheOldSet();

  void addBook_withoutAnIsbn_keysTheRowBelowTheIsbnRange();
  void addBook_withoutAnIsbn_keysEachRowSeparately();
  void addBook_withoutAnIsbn_keepsCountingPastCatalogBooks();

  void deleteBook_removesIt();
  void deleteBook_unknownIsbn_returnsFalse();
  void deleteBook_cascadesToCharactersAndSessions();

  void setGenres_replacesRatherThanMerges();
  void setGenres_trimsNamesAndSkipsBlanks();
  void setGenres_reusesAnExistingGenreRow();
  void setGenres_withAnEmptyList_clearsThem();
  void getGenres_areSortedByName();
  void getGenres_unknownBook_returnsNothing();

  void getCharacters_areOrderedById();
  void getCharacters_unknownBook_returnsNothing();

  void updatePagesRead_movesThePosition();
  void updatePagesRead_unknownIsbn_returnsFalse();

  void insertReadingSession_returnsTheNewId();
  void insertReadingSession_stampsTheMeasuredDuration();

  void getReadingSessions_areNewestFirst();
  void getReadingSessions_skipOpenEntries();
  void getReadingSessions_unknownBook_returnsNothing();
  void getAllReadingSessions_spanEveryBook();
  void getAllReadingSessions_skipOpenEntries();
  void getAllReadingSessions_emptyJournal_returnsNothing();

  void deleteReadingSession_removesOnlyThatEntry();
  void deleteReadingSession_leavesThePositionAlone();
  void deleteReadingSession_unknownId_stillReportsSuccess();

private:
  int countOf(const QString &table) const;
  bool foreignKeysEnforced() const;

  TempLibrary _library;
};

void BookTableTest::initTestCase() {
  // Resources of a static lib can be stripped by the linker — see main.cpp.
  Q_INIT_RESOURCE(db_scripts);
}

void BookTableTest::init() { QVERIFY(_library.open()); }

void BookTableTest::cleanup() { _library.close(); }

int BookTableTest::countOf(const QString &table) const {
  SqlQueryBuilder query;
  query.selectCount().from(table);
  const auto rows = _library.db()->select(query);
  return rows.isEmpty() ? -1 : rows.first().first().toInt();
}

// init.sql opens with `PRAGMA foreign_keys = ON`, but SQLite ignores that
// pragma inside a transaction and DatabaseManager runs every statement in one —
// so whether the schema's cascades actually fire is a property of the running
// build, not something a test can assume.
bool BookTableTest::foreignKeysEnforced() const {
  const bool orphanAccepted =
      _library.db()->exec(u"INSERT INTO book_characters (book_isbn, name) VALUES (1, 'probe')"_s);
  if (orphanAccepted) {
    _library.db()->exec(u"DELETE FROM book_characters WHERE book_isbn = 1"_s);
  }
  return !orphanAccepted;
}

void BookTableTest::addBook_storesEveryField() {
  BookDTO book = makeBook(kIsbn, u"Refactoring"_s, BookStatus::InProgress);
  book.authorName = u"Martin Fowler"_s;
  book.year = 1999;
  book.publisherName = u"Addison-Wesley"_s;
  book.description = u"Improving the design of existing code"_s;
  book.coverUrl = u"https://example.invalid/cover.jpg"_s;
  book.isHardcover = true;
  book.totalPages = 448;
  book.pagesRead = 120;
  book.globalRating = 4.5;
  book.localRating = 4.0;
  book.userRating = 9;
  book.inWishList = true;

  QCOMPARE(_library.books()->addBook(book), kIsbn);

  const auto stored = _library.books()->getAllBooks();
  QCOMPARE(stored.size(), 1);

  const BookDTO &read = stored.first();
  QCOMPARE(read.isbn, book.isbn);
  QCOMPARE(read.name, book.name);
  QCOMPARE(read.authorName, book.authorName);
  QCOMPARE(read.year, book.year);
  QCOMPARE(read.publisherName, book.publisherName);
  QCOMPARE(read.description, book.description);
  QCOMPARE(read.coverUrl, book.coverUrl);
  QCOMPARE(read.isHardcover, book.isHardcover);
  QCOMPARE(read.typeName, book.typeName);
  QCOMPARE(read.totalPages, book.totalPages);
  QCOMPARE(read.pagesRead, book.pagesRead);
  QCOMPARE(read.globalRating, book.globalRating);
  QCOMPARE(read.localRating, book.localRating);
  QCOMPARE(read.userRating, book.userRating);
  QCOMPARE(read.status, book.status);
  QCOMPARE(read.inWishList, book.inWishList);
  QCOMPARE(read.language, book.language);
}

void BookTableTest::addBook_returnsTheIsbnItWasGiven() {
  // isbn is the primary key supplied by the caller, not an autoincrement.
  QCOMPARE(_library.books()->addBook(makeBook(kIsbn, u"Refactoring"_s)), kIsbn);
}

void BookTableTest::addBook_duplicateIsbn_isRejected() {
  QCOMPARE(_library.books()->addBook(makeBook(kIsbn, u"Refactoring"_s)), kIsbn);
  QCOMPARE(_library.books()->addBook(makeBook(kIsbn, u"Refactoring, again"_s)), 0LL);
  QCOMPARE(countOf(u"books"_s), 1);
}

void BookTableTest::addBook_withGenres_linksThem() {
  BookDTO book = makeBook(kIsbn, u"Refactoring"_s);
  book.genres = {u"Programming"_s, u"Software"_s};

  QCOMPARE(_library.books()->addBook(book), kIsbn);
  QCOMPARE(_library.books()->getGenres(kIsbn), QStringList({u"Programming"_s, u"Software"_s}));
}

void BookTableTest::addBook_violatingASchemaCheck_isRejected() {
  // userRating is constrained to 0..10.
  BookDTO book = makeBook(kIsbn, u"Refactoring"_s);
  book.userRating = 42;

  QCOMPARE(_library.books()->addBook(book), 0LL);
  QCOMPARE(countOf(u"books"_s), 0);
}

void BookTableTest::addBook_withoutAPageCount_storesNull() {
  // The schema rejects totalPages = 0 outright, and that is exactly what the
  // DTO carries for "length unknown" — it has to reach the column as NULL.
  BookDTO book = makeBook(kIsbn, u"Refactoring"_s);
  book.totalPages = 0;

  QCOMPARE(_library.books()->addBook(book), kIsbn);

  SqlQueryBuilder query;
  query.select({u"totalPages"_s}).from(u"books"_s).where(u"isbn = ?"_s).values({kIsbn});
  const auto rows = _library.db()->select(query);
  QCOMPARE(rows.size(), 1);
  QVERIFY(rows.first().value(u"totalPages"_s).isNull());
  QCOMPARE(_library.books()->getAllBooks().first().totalPages, 0);
}

void BookTableTest::addBook_keepsTheCustomFlag() {
  BookDTO book = makeBook(kIsbn, u"My own notes"_s);
  book.isCustom = true;

  QCOMPARE(_library.books()->addBook(book), kIsbn);
  QVERIFY(_library.books()->getAllBooks().first().isCustom);
}

void BookTableTest::getAllBooks_onAnEmptyLibrary_returnsNothing() {
  QVERIFY(_library.books()->getAllBooks().isEmpty());
}

void BookTableTest::getAllBooks_attachesGenresToTheRightBook() {
  BookDTO first = makeBook(kIsbn, u"Refactoring"_s);
  first.genres = {u"Software"_s};
  BookDTO second = makeBook(kOtherIsbn, u"Effective Modern C++"_s);
  second.genres = {u"C++"_s, u"Reference"_s};

  QCOMPARE(_library.books()->addBook(first), kIsbn);
  QCOMPARE(_library.books()->addBook(second), kOtherIsbn);

  const auto stored = _library.books()->getAllBooks();
  QCOMPARE(stored.size(), 2);
  for (const BookDTO &book : stored) {
    if (book.isbn == kIsbn) {
      QCOMPARE(book.genres, QStringList({u"Software"_s}));
    } else {
      QCOMPARE(book.genres, QStringList({u"C++"_s, u"Reference"_s}));
    }
  }
}

void BookTableTest::updateBook_writesTheNewValues() {
  QCOMPARE(_library.books()->addBook(makeBook(kIsbn, u"Refactoring"_s)), kIsbn);

  BookDTO book = makeBook(kIsbn, u"Refactoring, 2nd Edition"_s, BookStatus::Finished);
  book.pagesRead = 448;
  book.totalPages = 448;
  QVERIFY(_library.books()->updateBook(book));

  const auto stored = _library.books()->getAllBooks();
  QCOMPARE(stored.size(), 1);
  QCOMPARE(stored.first().name, u"Refactoring, 2nd Edition"_s);
  QCOMPARE(stored.first().status, static_cast<int>(BookStatus::Finished));
  QCOMPARE(stored.first().pagesRead, 448);
}

void BookTableTest::updateBook_unknownIsbn_returnsFalse() {
  QVERIFY(!_library.books()->updateBook(makeBook(kIsbn, u"Refactoring"_s)));
}

void BookTableTest::updateBook_withGenres_replacesTheOldSet() {
  BookDTO book = makeBook(kIsbn, u"Refactoring"_s);
  book.genres = {u"Software"_s, u"Programming"_s};
  QCOMPARE(_library.books()->addBook(book), kIsbn);

  book.genres = {u"Craft"_s};
  QVERIFY(_library.books()->updateBook(book));
  QCOMPARE(_library.books()->getGenres(kIsbn), QStringList({u"Craft"_s}));
}

void BookTableTest::updateBook_withoutGenres_leavesTheOldSet() {
  // An empty list means "not supplied", not "clear them" — clearing goes
  // through setGenres.
  BookDTO book = makeBook(kIsbn, u"Refactoring"_s);
  book.genres = {u"Software"_s};
  QCOMPARE(_library.books()->addBook(book), kIsbn);

  book.genres.clear();
  QVERIFY(_library.books()->updateBook(book));
  QCOMPARE(_library.books()->getGenres(kIsbn), QStringList({u"Software"_s}));
}

void BookTableTest::addBook_keepsThePdfColumns() {
  BookDTO book = makeBook(kIsbn, u"Refactoring"_s);
  book.pdfPath = u"C:/data/pdfs/9780201616224.pdf"_s;
  book.pdfSource = 2;

  QCOMPARE(_library.books()->addBook(book), kIsbn);

  const BookDTO stored = _library.books()->getAllBooks().first();
  QCOMPARE(stored.pdfPath, book.pdfPath);
  QCOMPARE(stored.pdfSource, 2);
}

void BookTableTest::updateBook_writesThePdfColumns() {
  // Attaching and removing a pdf both go through updateBook.
  QCOMPARE(_library.books()->addBook(makeBook(kIsbn, u"Refactoring"_s)), kIsbn);

  BookDTO book = makeBook(kIsbn, u"Refactoring"_s);
  book.pdfPath = u"C:/data/pdfs/9780201616224.pdf"_s;
  book.pdfSource = 2;
  QVERIFY(_library.books()->updateBook(book));
  QCOMPARE(_library.books()->getAllBooks().first().pdfSource, 2);

  book.pdfPath.clear();
  book.pdfSource = 0;
  QVERIFY(_library.books()->updateBook(book));

  const BookDTO stored = _library.books()->getAllBooks().first();
  QVERIFY(stored.pdfPath.isEmpty());
  QCOMPARE(stored.pdfSource, 0);
}

void BookTableTest::addBook_withoutAnIsbn_keysTheRowBelowTheIsbnRange() {
  // No ISBN is invented for a book that has none — the row gets a small local
  // key that could never be read as an ISBN.
  BookDTO book = makeBook(0, u"My own notes"_s);
  book.isCustom = true;

  const qint64 key = _library.books()->addBook(book);

  QCOMPARE(key, 1LL);
  QCOMPARE(_library.books()->getAllBooks().first().isbn, key);
}

void BookTableTest::addBook_withoutAnIsbn_keysEachRowSeparately() {
  BookDTO first = makeBook(0, u"First"_s);
  BookDTO second = makeBook(0, u"Second"_s);

  const qint64 firstKey = _library.books()->addBook(first);
  const qint64 secondKey = _library.books()->addBook(second);

  QVERIFY(firstKey > 0);
  QVERIFY(secondKey > 0);
  QVERIFY(firstKey != secondKey);
  QCOMPARE(countOf(u"books"_s), 2);
}

void BookTableTest::addBook_withoutAnIsbn_keepsCountingPastCatalogBooks() {
  // A library full of real ISBNs must not push the local keys into their range:
  // a later import of the book with that ISBN would collide.
  QCOMPARE(_library.books()->addBook(makeBook(kIsbn, u"Refactoring"_s)), kIsbn);
  QCOMPARE(_library.books()->addBook(makeBook(kOtherIsbn, u"Effective Modern C++"_s)), kOtherIsbn);

  const qint64 key = _library.books()->addBook(makeBook(0, u"My own notes"_s));

  QCOMPARE(key, 1LL);
}

void BookTableTest::deleteBook_removesIt() {
  QCOMPARE(_library.books()->addBook(makeBook(kIsbn, u"Refactoring"_s)), kIsbn);

  QVERIFY(_library.books()->deleteBook(kIsbn));
  QVERIFY(_library.books()->getAllBooks().isEmpty());
}

void BookTableTest::deleteBook_unknownIsbn_returnsFalse() { QVERIFY(!_library.books()->deleteBook(kIsbn)); }

void BookTableTest::deleteBook_cascadesToCharactersAndSessions() {
  if (!foreignKeysEnforced()) {
    QSKIP("SQLite is not enforcing foreign keys on this connection, so the schema's cascades cannot fire");
  }

  BookDTO book = makeBook(kIsbn, u"Refactoring"_s);
  book.genres = {u"Software"_s};
  QCOMPARE(_library.books()->addBook(book), kIsbn);
  QVERIFY(_library.db()->exec(u"INSERT INTO book_characters (book_isbn, name) VALUES (9780201616224, 'Extract')"_s));
  QVERIFY(_library.books()->insertReadingSession(kIsbn, 0, 90, 600) > 0);

  QVERIFY(_library.books()->deleteBook(kIsbn));

  QCOMPARE(countOf(u"book_characters"_s), 0);
  QCOMPARE(countOf(u"reading_sessions"_s), 0);
  QCOMPARE(countOf(u"book_genres"_s), 0);
  // The genre vocabulary itself is shared, so it stays behind.
  QCOMPARE(countOf(u"genres"_s), 1);
}

void BookTableTest::setGenres_replacesRatherThanMerges() {
  QCOMPARE(_library.books()->addBook(makeBook(kIsbn, u"Refactoring"_s)), kIsbn);

  QVERIFY(_library.books()->setGenres(kIsbn, {u"Software"_s, u"Programming"_s}));
  QVERIFY(_library.books()->setGenres(kIsbn, {u"Craft"_s}));

  QCOMPARE(_library.books()->getGenres(kIsbn), QStringList({u"Craft"_s}));
}

void BookTableTest::setGenres_trimsNamesAndSkipsBlanks() {
  QCOMPARE(_library.books()->addBook(makeBook(kIsbn, u"Refactoring"_s)), kIsbn);

  QVERIFY(_library.books()->setGenres(kIsbn, {u"  Software  "_s, u"   "_s, QString{}}));

  QCOMPARE(_library.books()->getGenres(kIsbn), QStringList({u"Software"_s}));
}

void BookTableTest::setGenres_reusesAnExistingGenreRow() {
  QCOMPARE(_library.books()->addBook(makeBook(kIsbn, u"Refactoring"_s)), kIsbn);
  QCOMPARE(_library.books()->addBook(makeBook(kOtherIsbn, u"Effective Modern C++"_s)), kOtherIsbn);

  QVERIFY(_library.books()->setGenres(kIsbn, {u"Software"_s}));
  QVERIFY(_library.books()->setGenres(kOtherIsbn, {u"Software"_s}));

  QCOMPARE(countOf(u"genres"_s), 1);
  QCOMPARE(countOf(u"book_genres"_s), 2);
}

void BookTableTest::setGenres_withAnEmptyList_clearsThem() {
  QCOMPARE(_library.books()->addBook(makeBook(kIsbn, u"Refactoring"_s)), kIsbn);
  QVERIFY(_library.books()->setGenres(kIsbn, {u"Software"_s}));

  QVERIFY(_library.books()->setGenres(kIsbn, {}));

  QVERIFY(_library.books()->getGenres(kIsbn).isEmpty());
}

void BookTableTest::getGenres_areSortedByName() {
  QCOMPARE(_library.books()->addBook(makeBook(kIsbn, u"Refactoring"_s)), kIsbn);
  QVERIFY(_library.books()->setGenres(kIsbn, {u"Software"_s, u"Craft"_s, u"Programming"_s}));

  QCOMPARE(_library.books()->getGenres(kIsbn), QStringList({u"Craft"_s, u"Programming"_s, u"Software"_s}));
}

void BookTableTest::getGenres_unknownBook_returnsNothing() { QVERIFY(_library.books()->getGenres(kIsbn).isEmpty()); }

void BookTableTest::getCharacters_areOrderedById() {
  QCOMPARE(_library.books()->addBook(makeBook(kIsbn, u"Refactoring"_s)), kIsbn);
  QVERIFY(_library.db()->exec(u"INSERT INTO book_characters (book_isbn, name, role) VALUES "
                              "(9780201616224, 'Extract Method', 'Composing'), "
                              "(9780201616224, 'Inline Method', NULL)"_s));

  const auto characters = _library.books()->getCharacters(kIsbn);
  QCOMPARE(characters.size(), 2);
  QCOMPARE(characters.first().name, u"Extract Method"_s);
  QCOMPARE(characters.first().role, u"Composing"_s);
  QVERIFY(characters.first().id < characters.last().id);
  QVERIFY(characters.last().role.isEmpty());
}

void BookTableTest::getCharacters_unknownBook_returnsNothing() {
  QVERIFY(_library.books()->getCharacters(kIsbn).isEmpty());
}

void BookTableTest::updatePagesRead_movesThePosition() {
  QCOMPARE(_library.books()->addBook(makeBook(kIsbn, u"Refactoring"_s)), kIsbn);

  QVERIFY(_library.books()->updatePagesRead(kIsbn, 120));

  QCOMPARE(_library.books()->getAllBooks().first().pagesRead, 120);
}

void BookTableTest::updatePagesRead_unknownIsbn_returnsFalse() {
  QVERIFY(!_library.books()->updatePagesRead(kIsbn, 120));
}

void BookTableTest::insertReadingSession_returnsTheNewId() {
  QCOMPARE(_library.books()->addBook(makeBook(kIsbn, u"Refactoring"_s)), kIsbn);

  const qint64 first = _library.books()->insertReadingSession(kIsbn, 0, 90, 600);
  const qint64 second = _library.books()->insertReadingSession(kIsbn, 90, 180, 900);

  QVERIFY(first > 0);
  QVERIFY(second > first);
}

void BookTableTest::insertReadingSession_stampsTheMeasuredDuration() {
  QCOMPARE(_library.books()->addBook(makeBook(kIsbn, u"Refactoring"_s)), kIsbn);
  QVERIFY(_library.books()->insertReadingSession(kIsbn, 0, 90, 600) > 0);

  const auto sessions = _library.books()->getReadingSessions(kIsbn);
  QCOMPARE(sessions.size(), 1);
  QCOMPARE(sessions.first().durationSeconds(), 600);
  QCOMPARE(sessions.first().pagesRead(), 90);
  // The row is closed the moment it is written — the timer already stopped.
  QVERIFY(sessions.first().endedAt.isValid());
}

void BookTableTest::getReadingSessions_areNewestFirst() {
  QCOMPARE(_library.books()->addBook(makeBook(kIsbn, u"Refactoring"_s)), kIsbn);
  QVERIFY(_library.db()->exec(u"INSERT INTO reading_sessions (book_isbn, started_at, ended_at, pages_from, pages_to) "
                              "VALUES (9780201616224, '2026-03-01T19:00:00Z', '2026-03-01T20:30:00Z', 0, 90), "
                              "(9780201616224, '2026-03-09T19:00:00Z', '2026-03-09T20:00:00Z', 180, 270), "
                              "(9780201616224, '2026-03-05T18:30:00Z', '2026-03-05T20:00:00Z', 90, 180)"_s));

  const auto sessions = _library.books()->getReadingSessions(kIsbn);
  QCOMPARE(sessions.size(), 3);
  QCOMPARE(sessions.at(0).pagesFrom, 180);
  QCOMPARE(sessions.at(1).pagesFrom, 90);
  QCOMPARE(sessions.at(2).pagesFrom, 0);
}

void BookTableTest::getReadingSessions_skipOpenEntries() {
  QCOMPARE(_library.books()->addBook(makeBook(kIsbn, u"Refactoring"_s)), kIsbn);
  QVERIFY(_library.db()->exec(u"INSERT INTO reading_sessions (book_isbn, started_at, ended_at, pages_from, pages_to) "
                              "VALUES (9780201616224, '2026-03-01T19:00:00Z', '2026-03-01T20:30:00Z', 0, 90), "
                              "(9780201616224, '2026-03-09T19:00:00Z', NULL, 90, NULL)"_s));

  const auto sessions = _library.books()->getReadingSessions(kIsbn);
  QCOMPARE(sessions.size(), 1);
  QCOMPARE(sessions.first().pagesFrom, 0);
}

void BookTableTest::getReadingSessions_unknownBook_returnsNothing() {
  QVERIFY(_library.books()->getReadingSessions(kIsbn).isEmpty());
}

void BookTableTest::getAllReadingSessions_spanEveryBook() {
  QCOMPARE(_library.books()->addBook(makeBook(kIsbn, u"Refactoring"_s)), kIsbn);
  QCOMPARE(_library.books()->addBook(makeBook(kOtherIsbn, u"Effective Modern C++"_s)), kOtherIsbn);
  QVERIFY(_library.books()->insertReadingSession(kIsbn, 0, 90, 600) > 0);
  QVERIFY(_library.books()->insertReadingSession(kIsbn, 90, 180, 900) > 0);
  QVERIFY(_library.books()->insertReadingSession(kOtherIsbn, 0, 40, 1200) > 0);

  const auto sessions = _library.books()->getAllReadingSessions();

  QCOMPARE(sessions.size(), 3);
  int seconds = 0;
  for (const auto &session : sessions) {
    seconds += session.durationSeconds();
  }
  QCOMPARE(seconds, 2700);

  // Each row says whose it is — the reading-statistics page dates a finished
  // book by its own last session.
  int ofOther = 0;
  for (const auto &session : sessions) {
    QVERIFY(session.bookIsbn == kIsbn || session.bookIsbn == kOtherIsbn);
    ofOther += session.bookIsbn == kOtherIsbn ? 1 : 0;
  }
  QCOMPARE(ofOther, 1);
}

void BookTableTest::getAllReadingSessions_skipOpenEntries() {
  // A row still in flight belongs to the timer cache, not to the journal.
  QCOMPARE(_library.books()->addBook(makeBook(kIsbn, u"Refactoring"_s)), kIsbn);
  QVERIFY(_library.db()->exec(u"INSERT INTO reading_sessions (book_isbn, started_at, ended_at, pages_from, pages_to) "
                              "VALUES (9780201616224, '2026-03-01T19:00:00Z', '2026-03-01T20:30:00Z', 0, 90), "
                              "(9780201616224, '2026-03-09T19:00:00Z', NULL, 90, NULL)"_s));

  const auto sessions = _library.books()->getAllReadingSessions();

  QCOMPARE(sessions.size(), 1);
  QCOMPARE(sessions.first().pagesFrom, 0);
}

void BookTableTest::getAllReadingSessions_emptyJournal_returnsNothing() {
  QCOMPARE(_library.books()->addBook(makeBook(kIsbn, u"Refactoring"_s)), kIsbn);

  QVERIFY(_library.books()->getAllReadingSessions().isEmpty());
}

void BookTableTest::deleteReadingSession_removesOnlyThatEntry() {
  QCOMPARE(_library.books()->addBook(makeBook(kIsbn, u"Refactoring"_s)), kIsbn);
  const qint64 first = _library.books()->insertReadingSession(kIsbn, 0, 90, 600);
  const qint64 second = _library.books()->insertReadingSession(kIsbn, 90, 180, 900);
  QVERIFY(first > 0);
  QVERIFY(second > 0);

  QVERIFY(_library.books()->deleteReadingSession(first));

  const auto sessions = _library.books()->getReadingSessions(kIsbn);
  QCOMPARE(sessions.size(), 1);
  QCOMPARE(sessions.first().id, second);
}

void BookTableTest::deleteReadingSession_leavesThePositionAlone() {
  // books.pagesRead is the reading position, not a sum over the journal.
  BookDTO book = makeBook(kIsbn, u"Refactoring"_s);
  book.pagesRead = 180;
  QCOMPARE(_library.books()->addBook(book), kIsbn);
  const qint64 id = _library.books()->insertReadingSession(kIsbn, 90, 180, 900);
  QVERIFY(id > 0);

  QVERIFY(_library.books()->deleteReadingSession(id));

  QCOMPARE(_library.books()->getAllBooks().first().pagesRead, 180);
}

void BookTableTest::deleteReadingSession_unknownId_stillReportsSuccess() {
  // The end state the caller asked for holds either way.
  QVERIFY(_library.books()->deleteReadingSession(999999));
}

QTEST_GUILESS_MAIN(BookTableTest)
#include "BookTableTest.moc"
