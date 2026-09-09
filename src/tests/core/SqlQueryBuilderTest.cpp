#include "core/SqlQueryBuilder.hpp"

#include <QTest>

using Qt::StringLiterals::operator""_s;

using readary::core::SqlQueryBuilder;

class SqlQueryBuilderTest : public QObject {
  Q_OBJECT

private slots:
  void select_withoutColumns_selectsEverything();
  void select_withStarColumn_selectsEverything();
  void select_withPaddedStarColumn_selectsEverything();
  void select_joinsColumns();
  void select_replacesAnythingBuiltBefore();
  void selectCount_countsRows();
  void from_appendsAlias();
  void from_withoutAlias_omitsIt();
  void where_appendsCondition();
  void set_placesOnePlaceholderPerColumn();
  void leftJoin_and_on_composeTheClause();
  void orderBy_defaultsToAscending();
  void orderBy_chained_extendsOneClause();
  void insertInto_buildsPlaceholders();
  void insertOrIgnoreInto_usesTheIgnoringVerb();
  void insertInto_discardsValuesBoundEarlier();
  void update_and_deleteFrom_startTheStatement();
  void values_areReturnedInOrder();
  void values_moveOverload_takesOwnership();
  void values_replaceRatherThanAppend();
  void fullSelect_readsAsOneStatement();
};

void SqlQueryBuilderTest::select_withoutColumns_selectsEverything() {
  SqlQueryBuilder query;
  QCOMPARE(query.select().build(), u"SELECT *"_s);
}

void SqlQueryBuilderTest::select_withStarColumn_selectsEverything() {
  SqlQueryBuilder query;
  QCOMPARE(query.select({u"*"_s}).build(), u"SELECT *"_s);
}

void SqlQueryBuilderTest::select_withPaddedStarColumn_selectsEverything() {
  SqlQueryBuilder query;
  QCOMPARE(query.select({u"  *  "_s}).build(), u"SELECT *"_s);
}

void SqlQueryBuilderTest::select_joinsColumns() {
  SqlQueryBuilder query;
  QCOMPARE(query.select({u"isbn"_s, u"name"_s, u"author"_s}).build(), u"SELECT isbn, name, author"_s);
}

void SqlQueryBuilderTest::select_replacesAnythingBuiltBefore() {
  // select() assigns rather than appends, so it always starts a fresh statement.
  SqlQueryBuilder query;
  query.select({u"isbn"_s}).from(u"books"_s);
  QCOMPARE(query.select({u"name"_s}).build(), u"SELECT name"_s);
}

void SqlQueryBuilderTest::selectCount_countsRows() {
  SqlQueryBuilder query;
  QCOMPARE(query.selectCount().from(u"books"_s).build(), u"SELECT COUNT(*) FROM books"_s);
}

void SqlQueryBuilderTest::from_appendsAlias() {
  SqlQueryBuilder query;
  QCOMPARE(query.select({u"b.isbn"_s}).from(u"books"_s, u"b"_s).build(), u"SELECT b.isbn FROM books b"_s);
}

void SqlQueryBuilderTest::from_withoutAlias_omitsIt() {
  SqlQueryBuilder query;
  QCOMPARE(query.select().from(u"books"_s).build(), u"SELECT * FROM books"_s);
}

void SqlQueryBuilderTest::where_appendsCondition() {
  SqlQueryBuilder query;
  QCOMPARE(query.select().from(u"books"_s).where(u"isbn = ?"_s).build(), u"SELECT * FROM books WHERE isbn = ?"_s);
}

void SqlQueryBuilderTest::set_placesOnePlaceholderPerColumn() {
  SqlQueryBuilder query;
  QCOMPARE(query.update(u"books"_s).set({u"name"_s, u"author"_s, u"year"_s}).where(u"isbn = ?"_s).build(),
           u"UPDATE books SET name = ?, author = ?, year = ? WHERE isbn = ?"_s);
}

void SqlQueryBuilderTest::leftJoin_and_on_composeTheClause() {
  SqlQueryBuilder query;
  QCOMPARE(query.select({u"g.name"_s})
               .from(u"book_genres"_s, u"bg"_s)
               .leftJoin(u"genres"_s, u"g"_s)
               .on(u"g.id = bg.genre_id"_s)
               .build(),
           u"SELECT g.name FROM book_genres bg LEFT JOIN genres g ON g.id = bg.genre_id"_s);
}

void SqlQueryBuilderTest::orderBy_defaultsToAscending() {
  SqlQueryBuilder query;
  QCOMPARE(query.select().from(u"books"_s).orderBy(u"name"_s).build(), u"SELECT * FROM books ORDER BY name ASC"_s);
}

void SqlQueryBuilderTest::orderBy_chained_extendsOneClause() {
  // A second ORDER BY keyword would be a syntax error; chained calls have to
  // become one comma-separated clause.
  SqlQueryBuilder query;
  QCOMPARE(query.select()
               .from(u"reading_sessions"_s)
               .orderBy(u"started_at"_s, u"DESC"_s)
               .orderBy(u"id"_s, u"DESC"_s)
               .build(),
           u"SELECT * FROM reading_sessions ORDER BY started_at DESC, id DESC"_s);
}

void SqlQueryBuilderTest::insertInto_buildsPlaceholders() {
  SqlQueryBuilder query;
  QCOMPARE(query.insertInto(u"genres"_s, {u"name"_s, u"slug"_s}).build(),
           u"INSERT INTO genres (name, slug) VALUES (?, ?)"_s);
}

void SqlQueryBuilderTest::insertOrIgnoreInto_usesTheIgnoringVerb() {
  SqlQueryBuilder query;
  QCOMPARE(query.insertOrIgnoreInto(u"genres"_s, {u"name"_s}).build(),
           u"INSERT OR IGNORE INTO genres (name) VALUES (?)"_s);
}

void SqlQueryBuilderTest::insertInto_discardsValuesBoundEarlier() {
  // The placeholder count comes from the column list, so values bound for some
  // earlier statement would silently mis-bind.
  SqlQueryBuilder query;
  query.values({1, 2, 3}).insertInto(u"genres"_s, {u"name"_s});
  QVERIFY(query.getValues().isEmpty());
}

void SqlQueryBuilderTest::update_and_deleteFrom_startTheStatement() {
  SqlQueryBuilder update;
  QCOMPARE(update.update(u"books"_s).build(), u"UPDATE books"_s);

  SqlQueryBuilder remove;
  QCOMPARE(remove.deleteFrom(u"books"_s).where(u"isbn = ?"_s).build(), u"DELETE FROM books WHERE isbn = ?"_s);
}

void SqlQueryBuilderTest::values_areReturnedInOrder() {
  SqlQueryBuilder query;
  query.select().from(u"books"_s).where(u"isbn = ? AND status = ?"_s).values({9780201616224LL, 3});

  const QVariantList &bound = query.getValues();
  QCOMPARE(bound.size(), 2);
  QCOMPARE(bound.at(0).toLongLong(), 9780201616224LL);
  QCOMPARE(bound.at(1).toInt(), 3);
}

void SqlQueryBuilderTest::values_moveOverload_takesOwnership() {
  QVariantList source{u"Dune"_s, 1965};
  SqlQueryBuilder query;
  query.values(std::move(source));

  QCOMPARE(query.getValues().size(), 2);
  QCOMPARE(query.getValues().at(0).toString(), u"Dune"_s);
}

void SqlQueryBuilderTest::values_replaceRatherThanAppend() {
  SqlQueryBuilder query;
  query.values({1, 2}).values({3});

  QCOMPARE(query.getValues().size(), 1);
  QCOMPARE(query.getValues().at(0).toInt(), 3);
}

void SqlQueryBuilderTest::fullSelect_readsAsOneStatement() {
  SqlQueryBuilder query;
  query.select({u"b.isbn"_s, u"g.name"_s})
      .from(u"books"_s, u"b"_s)
      .leftJoin(u"book_genres"_s, u"bg"_s)
      .on(u"bg.book_isbn = b.isbn"_s)
      .where(u"b.status = ?"_s)
      .orderBy(u"b.name"_s)
      .values({3});

  QCOMPARE(query.build(), u"SELECT b.isbn, g.name FROM books b LEFT JOIN book_genres bg ON bg.book_isbn = b.isbn "
                          "WHERE b.status = ? ORDER BY b.name ASC"_s);
}

QTEST_GUILESS_MAIN(SqlQueryBuilderTest)
#include "SqlQueryBuilderTest.moc"
