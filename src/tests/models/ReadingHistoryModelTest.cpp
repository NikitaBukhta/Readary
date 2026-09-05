#include "models/books/ReadingHistoryModel.hpp"
#include "core/DatabaseManager.hpp"
#include "core/SqlQueryBuilder.hpp"
#include "services/BookTable.hpp"

#include <QSignalSpy>
#include <QTest>
#include <memory>

using Qt::StringLiterals::operator""_s;

namespace core = readary::core;

using readary::core::DatabaseManager;
using readary::models::ReadingHistoryModel;
using readary::services::BookTable;

namespace {

constexpr qint64 kIsbn = 9780201616224LL;
constexpr qint64 kUnreadIsbn = 9781491903995LL;

// Seven sessions inserted out of chronological order so ordering is exercised,
// plus one open (ended_at IS NULL) row that must never surface.
constexpr auto kSeedSessions = R"(
INSERT INTO reading_sessions (book_isbn, started_at, ended_at, pages_from, pages_to) VALUES
  (9780201616224, '2026-03-05T18:30:00Z', '2026-03-05T20:00:00Z',  90, 180),
  (9780201616224, '2026-03-01T19:00:00Z', '2026-03-01T20:30:00Z',   0,  90),
  (9780201616224, '2026-03-12T19:00:00Z', '2026-03-12T19:30:00Z', 270, 300),
  (9780201616224, '2026-03-09T19:00:00Z', '2026-03-09T20:00:00Z', 180, 270),
  (9780201616224, '2026-03-15T19:00:00Z', '2026-03-15T19:20:00Z', 300, 320),
  (9780201616224, '2026-03-18T19:00:00Z', '2026-03-18T19:10:00Z', 320, 340),
  (9780201616224, '2026-03-21T19:00:00Z', '2026-03-21T19:12:00Z', 340, 352),
  (9780201616224, '2026-03-24T19:00:00Z', NULL,                  352, NULL)
)";

} // namespace

class ReadingHistoryModelTest : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();

  void sessions_areOrderedNewestFirst();
  void sessions_skipStillRunningEntries();
  void rows_exposeDerivedPagesAndDuration();
  void model_startsAtOnePage_thenExpandsAndCollapses();
  void summary_countsWholeJournalNotVisibleRows();
  void reopeningSameBook_keepsExpansion();
  void switchingBook_collapsesBackToFirstPage();
  void unknownBook_hasNoRows();
  void deletingSession_dropsItFromTheJournalOnly();
  void deletingUnknownSession_isANoOp();

private:
  std::shared_ptr<DatabaseManager> _db;
  std::shared_ptr<BookTable> _bookTable;
};

void ReadingHistoryModelTest::initTestCase() {
  // Resources of a static lib can be stripped by the linker — see main.cpp.
  Q_INIT_RESOURCE(db_scripts);

  _db = std::make_shared<DatabaseManager>(u":memory:"_s);
  QVERIFY(_db->open());
  QVERIFY(_db->runScript(u":/db/init.sql"_s));

  // reading_sessions.book_isbn is a foreign key and init.sql turns enforcement on.
  QVERIFY(_db->exec(u"INSERT INTO books (isbn, name, author, totalPages, pagesRead, status) "
                    "VALUES (9780201616224, 'Refactoring', 'Fowler', 352, 352, 3)"_s));
  QVERIFY(_db->exec(u"INSERT INTO books (isbn, name, author) "
                    "VALUES (9781491903995, 'Effective Modern C++', 'Meyers')"_s));
  QVERIFY(_db->exec(QString::fromUtf8(kSeedSessions)));

  _bookTable = std::make_shared<BookTable>(_db);
}

void ReadingHistoryModelTest::sessions_areOrderedNewestFirst() {
  const auto sessions = _bookTable->getReadingSessions(kIsbn);

  QCOMPARE(static_cast<int>(sessions.size()), 7);
  for (qsizetype i = 1; i < sessions.size(); ++i) {
    QVERIFY2(sessions.at(i - 1).startedAt >= sessions.at(i).startedAt, "sessions must come back newest first");
  }
  QCOMPARE(sessions.first().pagesTo, 352);
  QCOMPARE(sessions.last().pagesFrom, 0);
}

void ReadingHistoryModelTest::sessions_skipStillRunningEntries() {
  const auto sessions = _bookTable->getReadingSessions(kIsbn);

  for (const auto &session : sessions) {
    QVERIFY2(session.endedAt.isValid(), "an open session belongs to the timer cache, not the history");
  }
}

void ReadingHistoryModelTest::rows_exposeDerivedPagesAndDuration() {
  ReadingHistoryModel model{_bookTable};
  model.setBookIsbn(kIsbn);

  const QModelIndex newest = model.index(0, 0);
  QCOMPARE(newest.data(ReadingHistoryModel::PagesFromRole).toInt(), 340);
  QCOMPARE(newest.data(ReadingHistoryModel::PagesToRole).toInt(), 352);
  QCOMPARE(newest.data(ReadingHistoryModel::PagesReadRole).toInt(), 12);
  QCOMPARE(newest.data(ReadingHistoryModel::DurationSecondsRole).toInt(), 12 * 60);
  QVERIFY(newest.data(ReadingHistoryModel::StartedAtRole).toDateTime().isValid());
  QVERIFY(newest.data(ReadingHistoryModel::EndedAtRole).toDateTime().isValid());
}

void ReadingHistoryModelTest::model_startsAtOnePage_thenExpandsAndCollapses() {
  ReadingHistoryModel model{_bookTable};
  model.setBookIsbn(kIsbn);

  QCOMPARE(model.rowCount(), 5);
  QVERIFY(model.canLoadMore());
  QVERIFY(!model.canHide());

  model.loadMore();
  QCOMPARE(model.rowCount(), 7);
  QVERIFY(!model.canLoadMore());
  QVERIFY(model.canHide());

  model.hide();
  QCOMPARE(model.rowCount(), 5);
  QVERIFY(model.canLoadMore());
  QVERIFY(!model.canHide());
}

void ReadingHistoryModelTest::summary_countsWholeJournalNotVisibleRows() {
  ReadingHistoryModel model{_bookTable};
  model.setBookIsbn(kIsbn);

  // The detail page hides the whole section on totalCount == 0.
  QCOMPARE(model.rowCount(), 5);
  QCOMPARE(model.totalCount(), 7);
}

void ReadingHistoryModelTest::reopeningSameBook_keepsExpansion() {
  ReadingHistoryModel model{_bookTable};
  model.setBookIsbn(kIsbn);
  model.loadMore();
  QCOMPARE(model.rowCount(), 7);

  QSignalSpy summarySpy{&model, &ReadingHistoryModel::summaryChanged};
  model.setBookIsbn(kIsbn);

  QCOMPARE(summarySpy.count(), 1);
  QCOMPARE(model.rowCount(), 7);
}

void ReadingHistoryModelTest::switchingBook_collapsesBackToFirstPage() {
  ReadingHistoryModel model{_bookTable};
  model.setBookIsbn(kIsbn);
  model.loadMore();

  model.setBookIsbn(kUnreadIsbn);
  QCOMPARE(model.rowCount(), 0);
  QCOMPARE(model.totalCount(), 0);

  model.setBookIsbn(kIsbn);
  QCOMPARE(model.rowCount(), 5);
}

void ReadingHistoryModelTest::unknownBook_hasNoRows() {
  ReadingHistoryModel model{_bookTable};
  model.setBookIsbn(0);

  QCOMPARE(model.rowCount(), 0);
  QVERIFY(!model.canLoadMore());
  QVERIFY(!model.canHide());
  QVERIFY(!model.index(0, 0).data(ReadingHistoryModel::IdRole).isValid());
}

void ReadingHistoryModelTest::deletingSession_dropsItFromTheJournalOnly() {
  ReadingHistoryModel model{_bookTable};
  model.setBookIsbn(kIsbn);
  model.loadMore();
  QCOMPARE(model.rowCount(), 7);

  const qint64 newest = model.index(0, 0).data(ReadingHistoryModel::IdRole).toLongLong();
  QVERIFY(newest > 0);
  QVERIFY(_bookTable->deleteReadingSession(newest));

  model.setBookIsbn(kIsbn);
  QCOMPARE(model.totalCount(), 6);
  QCOMPARE(model.rowCount(), 6); // still expanded
  QVERIFY(model.index(0, 0).data(ReadingHistoryModel::IdRole).toLongLong() != newest);

  // The journal is not the reading position: books.pagesRead must be untouched.
  core::SqlQueryBuilder pagesRead;
  pagesRead.select({u"pagesRead"_s}).from(u"books"_s).where(u"isbn = ?"_s).values({kIsbn});
  const auto rows = _db->select(pagesRead);
  QCOMPARE(rows.size(), 1);
  QCOMPARE(rows.first().value(u"pagesRead"_s).toInt(), 352);
}

void ReadingHistoryModelTest::deletingUnknownSession_isANoOp() {
  // Success, not failure: a cascade from deleting the book can get there first,
  // and the caller's end state holds either way.
  const qsizetype before = _bookTable->getReadingSessions(kIsbn).size();

  QVERIFY(_bookTable->deleteReadingSession(999999));

  QCOMPARE(_bookTable->getReadingSessions(kIsbn).size(), before);
}

QTEST_GUILESS_MAIN(ReadingHistoryModelTest)
#include "ReadingHistoryModelTest.moc"
