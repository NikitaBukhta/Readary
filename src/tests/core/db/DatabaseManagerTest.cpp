#include "core/db/DatabaseManager.hpp"
#include "core/db/SqlQueryBuilder.hpp"

#include <QTest>
#include <QTextStream>

#include <memory>

using Qt::StringLiterals::operator""_s;

using readary::core::DatabaseManager;
using readary::core::SqlQueryBuilder;

namespace {

// One connection name is baked into DatabaseManager, so every test holds its
// manager in a local scope and lets the destructor tear the connection down
// before the next one starts. Handed out by pointer because the class is
// deliberately neither copyable nor movable.
std::unique_ptr<DatabaseManager> makeOpenDatabase() {
  auto db = std::make_unique<DatabaseManager>(u":memory:"_s);
  db->open();
  return db;
}

bool createNumbersTable(DatabaseManager &db) {
  return db.exec(u"CREATE TABLE numbers (id INTEGER PRIMARY KEY AUTOINCREMENT, label TEXT NOT NULL, value INTEGER)"_s);
}

QString writeScript(const QTemporaryDir &dir, const QString &contents) {
  QString path = dir.filePath(u"script.sql"_s);
  QFile file{path};
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    return {};
  }
  QTextStream out{&file};
  out << contents;
  file.close();
  return path;
}

} // namespace

class DatabaseManagerTest : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();

  void open_inMemory_succeeds();
  void open_unwritablePath_fails();
  void runScript_missingFile_returnsFalse();
  void runScript_appliesTheProjectSchema();
  void runScript_skipsCommentsAndJoinsIndentedLines();
  void runScript_stripsTrailingCommentFromContinuationLine();
  void runScript_survivesAFailingStatement();
  void exec_reportsSqlErrors();
  void insert_returnsTheNewRowId();
  void insert_onFailure_returnsNegativeAndReportsError();
  void select_returnsRowsKeyedByColumnName();
  void select_bindsValuesInOrder();
  void select_onFailure_returnsNoRowsAndReportsError();
  void execute_returnsAffectedRowCount();
  void execute_matchingNothing_returnsZero();
  void execute_onFailure_returnsMinusOne();
  void clear_emptiesTheDatabase();
  void clear_removesTheBackingFile();
  void close_isIdempotent();
};

void DatabaseManagerTest::initTestCase() {
  // Resources of a static lib can be stripped by the linker — see main.cpp.
  Q_INIT_RESOURCE(db_scripts);
}

void DatabaseManagerTest::open_inMemory_succeeds() {
  DatabaseManager db{u":memory:"_s};
  QVERIFY(db.open());
}

void DatabaseManagerTest::open_unwritablePath_fails() {
  DatabaseManager db{u"/no/such/directory/readary.db"_s};
  QVERIFY(!db.open());
}

void DatabaseManagerTest::runScript_missingFile_returnsFalse() {
  const auto db = makeOpenDatabase();
  QVERIFY(!db->runScript(u":/db/does_not_exist.sql"_s));
}

void DatabaseManagerTest::runScript_appliesTheProjectSchema() {
  const auto db = makeOpenDatabase();
  QVERIFY(db->runScript(u":/db/init.sql"_s));

  SqlQueryBuilder tables;
  tables.select({u"name"_s}).from(u"sqlite_master"_s).where(u"type = 'table'"_s).orderBy(u"name"_s);

  QStringList names;
  for (const auto &row : db->select(tables)) {
    names << row.value(u"name"_s).toString();
  }
  QVERIFY(names.contains(u"books"_s));
  QVERIFY(names.contains(u"genres"_s));
  QVERIFY(names.contains(u"book_genres"_s));
  QVERIFY(names.contains(u"book_characters"_s));
  QVERIFY(names.contains(u"reading_sessions"_s));
}

void DatabaseManagerTest::runScript_skipsCommentsAndJoinsIndentedLines() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());

  // A statement is complete only when an unindented line ends in ';', so the
  // indented body below has to be folded into the CREATE that opens it.
  const QString path = writeScript(dir, uR"(-- a leading comment
CREATE TABLE folded (
  id    INTEGER PRIMARY KEY,
  label TEXT NOT NULL
);
-- another comment
INSERT INTO folded (id, label) VALUES (1, 'first');
)"_s);
  QVERIFY(!path.isEmpty());

  const auto db = makeOpenDatabase();
  QVERIFY(db->runScript(path));

  SqlQueryBuilder query;
  query.select({u"label"_s}).from(u"folded"_s);
  const auto rows = db->select(query);
  QCOMPARE(rows.size(), 1);
  QCOMPARE(rows.first().value(u"label"_s).toString(), u"first"_s);
}

void DatabaseManagerTest::runScript_stripsTrailingCommentFromContinuationLine() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());

  const QString path = writeScript(dir, uR"(CREATE TABLE annotated (
  id INTEGER PRIMARY KEY, -- the surrogate key
  label TEXT
);
)"_s);
  QVERIFY(!path.isEmpty());

  const auto db = makeOpenDatabase();
  QVERIFY(db->runScript(path));

  SqlQueryBuilder query;
  query.select({u"name"_s}).from(u"sqlite_master"_s).where(u"type = 'table' AND name = 'annotated'"_s);
  QCOMPARE(db->select(query).size(), 1);
}

void DatabaseManagerTest::runScript_survivesAFailingStatement() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());

  const QString path = writeScript(dir, uR"(NOT VALID SQL AT ALL;
CREATE TABLE survivor (id INTEGER PRIMARY KEY);
)"_s);
  QVERIFY(!path.isEmpty());

  const auto db = makeOpenDatabase();
  QVERIFY(db->runScript(path));

  SqlQueryBuilder query;
  query.select({u"name"_s}).from(u"sqlite_master"_s).where(u"type = 'table' AND name = 'survivor'"_s);
  QCOMPARE(db->select(query).size(), 1);
}

void DatabaseManagerTest::exec_reportsSqlErrors() {
  const auto db = makeOpenDatabase();

  QString error;
  QVERIFY(!db->exec(u"SELECT * FROM nothing_at_all"_s, &error));
  QVERIFY(!error.isEmpty());
}

void DatabaseManagerTest::insert_returnsTheNewRowId() {
  const auto db = makeOpenDatabase();
  QVERIFY(createNumbersTable(*db));

  SqlQueryBuilder first;
  first.insertInto(u"numbers"_s, {u"label"_s, u"value"_s}).values({u"one"_s, 1});
  QCOMPARE(db->insert(first), 1LL);

  SqlQueryBuilder second;
  second.insertInto(u"numbers"_s, {u"label"_s, u"value"_s}).values({u"two"_s, 2});
  QCOMPARE(db->insert(second), 2LL);
}

void DatabaseManagerTest::insert_onFailure_returnsNegativeAndReportsError() {
  const auto db = makeOpenDatabase();
  QVERIFY(createNumbersTable(*db));

  SqlQueryBuilder query;
  query.insertInto(u"numbers"_s, {u"label"_s}).values({QVariant{}}); // label is NOT NULL

  QString error;
  QCOMPARE(db->insert(query, &error), -1LL);
  QVERIFY(!error.isEmpty());
}

void DatabaseManagerTest::select_returnsRowsKeyedByColumnName() {
  const auto db = makeOpenDatabase();
  QVERIFY(createNumbersTable(*db));
  QVERIFY(db->exec(u"INSERT INTO numbers (label, value) VALUES ('one', 1), ('two', 2)"_s));

  SqlQueryBuilder query;
  query.select({u"label"_s, u"value"_s}).from(u"numbers"_s).orderBy(u"value"_s);

  const auto rows = db->select(query);
  QCOMPARE(rows.size(), 2);
  QCOMPARE(rows.first().keys(), QStringList({u"label"_s, u"value"_s}));
  QCOMPARE(rows.first().value(u"label"_s).toString(), u"one"_s);
  QCOMPARE(rows.last().value(u"value"_s).toInt(), 2);
}

void DatabaseManagerTest::select_bindsValuesInOrder() {
  const auto db = makeOpenDatabase();
  QVERIFY(createNumbersTable(*db));
  QVERIFY(db->exec(u"INSERT INTO numbers (label, value) VALUES ('one', 1), ('two', 2), ('three', 3)"_s));

  SqlQueryBuilder query;
  query.select({u"label"_s}).from(u"numbers"_s).where(u"value > ? AND value < ?"_s).values({1, 3});

  const auto rows = db->select(query);
  QCOMPARE(rows.size(), 1);
  QCOMPARE(rows.first().value(u"label"_s).toString(), u"two"_s);
}

void DatabaseManagerTest::select_onFailure_returnsNoRowsAndReportsError() {
  const auto db = makeOpenDatabase();

  SqlQueryBuilder query;
  query.select().from(u"nothing_at_all"_s);

  QString error;
  QVERIFY(db->select(query, &error).isEmpty());
  QVERIFY(!error.isEmpty());
}

void DatabaseManagerTest::execute_returnsAffectedRowCount() {
  const auto db = makeOpenDatabase();
  QVERIFY(createNumbersTable(*db));
  QVERIFY(db->exec(u"INSERT INTO numbers (label, value) VALUES ('one', 1), ('two', 1), ('three', 3)"_s));

  SqlQueryBuilder query;
  query.update(u"numbers"_s).set({u"value"_s}).where(u"value = ?"_s).values({9, 1});

  QCOMPARE(db->execute(query), 2);
}

void DatabaseManagerTest::execute_matchingNothing_returnsZero() {
  const auto db = makeOpenDatabase();
  QVERIFY(createNumbersTable(*db));

  SqlQueryBuilder query;
  query.deleteFrom(u"numbers"_s).where(u"id = ?"_s).values({404});

  QCOMPARE(db->execute(query), 0);
}

void DatabaseManagerTest::execute_onFailure_returnsMinusOne() {
  const auto db = makeOpenDatabase();

  SqlQueryBuilder query;
  query.deleteFrom(u"nothing_at_all"_s).where(u"id = ?"_s).values({1});

  QString error;
  QCOMPARE(db->execute(query, &error), -1);
  QVERIFY(!error.isEmpty());
}

void DatabaseManagerTest::clear_emptiesTheDatabase() {
  const auto db = makeOpenDatabase();
  QVERIFY(createNumbersTable(*db));

  QVERIFY(db->clear());

  // Reopening an in-memory database starts a brand new, empty one.
  SqlQueryBuilder query;
  query.select({u"name"_s}).from(u"sqlite_master"_s).where(u"type = 'table'"_s);
  QVERIFY(db->select(query).isEmpty());
}

void DatabaseManagerTest::clear_removesTheBackingFile() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString path = dir.filePath(u"library.db"_s);

  DatabaseManager db{path};
  QVERIFY(db.open());
  QVERIFY(createNumbersTable(db));
  QVERIFY(QFile::exists(path));

  QVERIFY(db.clear());

  SqlQueryBuilder query;
  query.select({u"name"_s}).from(u"sqlite_master"_s).where(u"type = 'table'"_s);
  QVERIFY(db.select(query).isEmpty());
}

void DatabaseManagerTest::close_isIdempotent() {
  const auto db = makeOpenDatabase();
  db->close();
  db->close();
  QVERIFY(db->open());
}

QTEST_GUILESS_MAIN(DatabaseManagerTest)
#include "DatabaseManagerTest.moc"
