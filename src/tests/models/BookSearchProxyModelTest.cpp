#include "models/books/BookSearchProxyModel.hpp"
#include "models/books/BookListModel.hpp"

#include <QAbstractListModel>
#include <QList>
#include <QString>
#include <QTest>
#include <QVariant>

namespace {

struct StubRow {
  int id;
  QString name;
  QString author;
  QString description;
};

class StubBookModel : public QAbstractListModel {
public:
  using Roles = readary::models::BookListModel::RolesEnum;

  explicit StubBookModel(QObject *parent = nullptr) : QAbstractListModel{parent} {}

  void setRows(QList<StubRow> rows) {
    beginResetModel();
    _rows = std::move(rows);
    endResetModel();
  }

  void updateRow(int index, StubRow row) {
    if (index < 0 || index >= _rows.size())
      return;
    _rows[index] = std::move(row);
    const QModelIndex idx = createIndex(index, 0);
    emit dataChanged(idx, idx);
  }

  int rowCount(const QModelIndex &parent = {}) const override {
    return parent.isValid() ? 0 : static_cast<int>(_rows.size());
  }

  QVariant data(const QModelIndex &index, int role) const override {
    if (!index.isValid() || index.row() < 0 || index.row() >= _rows.size())
      return {};
    const auto &r = _rows.at(index.row());
    switch (role) {
    case Roles::IsbnRole:
      return r.id;
    case Roles::NameRole:
      return r.name;
    case Roles::AuthorRole:
      return r.author;
    case Roles::DescriptionRole:
      return r.description;
    default:
      return {};
    }
  }

private:
  QList<StubRow> _rows;
};

QList<int> idsOf(const QAbstractItemModel &model) {
  QList<int> ids;
  ids.reserve(model.rowCount());
  for (int i = 0; i < model.rowCount(); ++i)
    ids << model.data(model.index(i, 0), readary::models::BookListModel::IsbnRole).toInt();
  return ids;
}

} // namespace

class BookSearchProxyModelTest : public QObject {
  Q_OBJECT

private slots:
  void emptyQuery_passesAllRowsInSourceOrder();
  void filterDropsNonMatchingRows();
  void matchIsCaseInsensitive();
  void searchesAcrossNameAuthorAndDescription();
  void nameMatchOutranksDescriptionMatch();
  void nameMatchOutranksAuthorMatch();
  void earlierMatchPositionRanksHigher();
  void higherCoverageRanksHigher();
  void exactNameMatchTopsList();
  void changingQueryUpdatesRanking();
  void clearingQueryRestoresAllRows();
  void dataChangedRefreshesFilterForUpdatedRow();
  void modelResetRecomputesEverything();
};

void BookSearchProxyModelTest::emptyQuery_passesAllRowsInSourceOrder() {
  StubBookModel src;
  src.setRows({
      {1, "Alpha", "A", "x"},
      {2, "Beta", "B", "y"},
      {3, "Gamma", "C", "z"},
  });

  readary::models::BookSearchProxyModel proxy;
  proxy.setSourceModel(&src);

  QCOMPARE(proxy.rowCount(), 3);
  QCOMPARE(idsOf(proxy), QList<int>({1, 2, 3}));
}

void BookSearchProxyModelTest::filterDropsNonMatchingRows() {
  StubBookModel src;
  src.setRows({
      {1, "Algorithms", "Cormen", "Classic CS textbook"},
      {2, "Cooking", "Smith", "Recipes for the home"},
      {3, "Algebra", "Lang", "Math foundations"},
  });

  readary::models::BookSearchProxyModel proxy;
  proxy.setSourceModel(&src);
  proxy.setSearchQuery("alg");

  QCOMPARE(proxy.rowCount(), 2);
  const auto ids = idsOf(proxy);
  QVERIFY(ids.contains(1));
  QVERIFY(ids.contains(3));
  QVERIFY(!ids.contains(2));
}

void BookSearchProxyModelTest::matchIsCaseInsensitive() {
  StubBookModel src;
  src.setRows({
      {1, "Foo Bar", "X", "Y"},
  });

  readary::models::BookSearchProxyModel proxy;
  proxy.setSourceModel(&src);
  proxy.setSearchQuery("FOO");

  QCOMPARE(proxy.rowCount(), 1);
}

void BookSearchProxyModelTest::searchesAcrossNameAuthorAndDescription() {
  StubBookModel src;
  src.setRows({
      {1, "Anna Karenina", "Leo Tolstoy", "Russian classic"},
      {2, "1984", "George Orwell", "Dystopian novel"},
      {3, "Crime and Punishment", "Fyodor Dostoevsky", "Philosophical fiction"},
  });

  readary::models::BookSearchProxyModel proxy;
  proxy.setSourceModel(&src);

  proxy.setSearchQuery("anna"); // matches name
  QCOMPARE(idsOf(proxy), QList<int>({1}));

  proxy.setSearchQuery("orwell"); // matches author
  QCOMPARE(idsOf(proxy), QList<int>({2}));

  proxy.setSearchQuery("philosophical"); // matches description
  QCOMPARE(idsOf(proxy), QList<int>({3}));
}

void BookSearchProxyModelTest::nameMatchOutranksDescriptionMatch() {
  StubBookModel src;
  src.setRows({
      // "novel" only in description (weight 20).
      {1, "War Memoir", "Author X", "A novel"},
      // "novel" in name (weight 100). Lower coverage but higher weight should win.
      {2, "Novel Approach", "Author Y", "Y"},
  });

  readary::models::BookSearchProxyModel proxy;
  proxy.setSourceModel(&src);
  proxy.setSearchQuery("novel");

  QCOMPARE(proxy.rowCount(), 2);
  QCOMPARE(idsOf(proxy), QList<int>({2, 1}));
}

void BookSearchProxyModelTest::nameMatchOutranksAuthorMatch() {
  StubBookModel src;
  src.setRows({
      // "leo" only in author (weight 60).
      {1, "Anna Karenina", "Leo Tolstoy", "X"},
      // "leo" in name (weight 100).
      {2, "Leonardo", "Other", "Y"},
  });

  readary::models::BookSearchProxyModel proxy;
  proxy.setSourceModel(&src);
  proxy.setSearchQuery("leo");

  QCOMPARE(proxy.rowCount(), 2);
  QCOMPARE(idsOf(proxy), QList<int>({2, 1}));
}

void BookSearchProxyModelTest::earlierMatchPositionRanksHigher() {
  StubBookModel src;
  // Same field length (25), same coverage. Only matchIdx differs.
  src.setRows({
      {1, "once long ago a cat lived", "X", "Y"}, // matchIdx = 16
      {2, "cat lived once long ago a", "X", "Y"}, // matchIdx = 0
  });

  readary::models::BookSearchProxyModel proxy;
  proxy.setSourceModel(&src);
  proxy.setSearchQuery("cat");

  QCOMPARE(proxy.rowCount(), 2);
  QCOMPARE(idsOf(proxy), QList<int>({2, 1}));
}

void BookSearchProxyModelTest::higherCoverageRanksHigher() {
  StubBookModel src;
  // Both match at index 0; difference is coverage.
  src.setRows({
      {1, "Java tutorial here", "X", "Y"}, // coverage = 4/18
      {2, "Java", "X", "Y"},               // coverage = 4/4 (exact field)
  });

  readary::models::BookSearchProxyModel proxy;
  proxy.setSourceModel(&src);
  proxy.setSearchQuery("java");

  QCOMPARE(proxy.rowCount(), 2);
  QCOMPARE(idsOf(proxy), QList<int>({2, 1}));
}

void BookSearchProxyModelTest::exactNameMatchTopsList() {
  StubBookModel src;
  src.setRows({
      {1, "The story of foo and bar", "X", "Y"}, // foo deep in name
      {2, "Foo Bar Baz", "X", "Y"},              // foo at start, partial coverage
      {3, "Foo", "X", "Y"},                      // exact field match
  });

  readary::models::BookSearchProxyModel proxy;
  proxy.setSourceModel(&src);
  proxy.setSearchQuery("foo");

  QCOMPARE(proxy.rowCount(), 3);
  QCOMPARE(idsOf(proxy).first(), 3);
}

void BookSearchProxyModelTest::changingQueryUpdatesRanking() {
  StubBookModel src;
  src.setRows({
      {1, "Apples", "X", "Y"},
      {2, "Bananas", "X", "Y"},
  });

  readary::models::BookSearchProxyModel proxy;
  proxy.setSourceModel(&src);

  proxy.setSearchQuery("apple");
  QCOMPARE(idsOf(proxy), QList<int>({1}));

  proxy.setSearchQuery("banana");
  QCOMPARE(idsOf(proxy), QList<int>({2}));
}

void BookSearchProxyModelTest::clearingQueryRestoresAllRows() {
  StubBookModel src;
  src.setRows({
      {1, "Apples", "X", "Y"},
      {2, "Bananas", "X", "Y"},
      {3, "Cherries", "X", "Y"},
  });

  readary::models::BookSearchProxyModel proxy;
  proxy.setSourceModel(&src);

  proxy.setSearchQuery("apple");
  QCOMPARE(proxy.rowCount(), 1);

  proxy.setSearchQuery("");
  QCOMPARE(proxy.rowCount(), 3);
  QCOMPARE(idsOf(proxy), QList<int>({1, 2, 3}));
}

void BookSearchProxyModelTest::dataChangedRefreshesFilterForUpdatedRow() {
  StubBookModel src;
  src.setRows({
      {1, "Java book", "X", "Y"},
      {2, "Cooking", "X", "Y"},
  });

  readary::models::BookSearchProxyModel proxy;
  proxy.setSourceModel(&src);
  proxy.setSearchQuery("java");

  QCOMPARE(idsOf(proxy), QList<int>({1}));

  // Mutating row 1 to contain "java" should make Qt re-run filterAcceptsRow on it,
  // which in turn refreshes the cache slot for that source row.
  src.updateRow(1, {2, "Java cookbook", "X", "Y"});

  QCOMPARE(proxy.rowCount(), 2);
  const auto ids = idsOf(proxy);
  QVERIFY(ids.contains(1));
  QVERIFY(ids.contains(2));
}

void BookSearchProxyModelTest::modelResetRecomputesEverything() {
  StubBookModel src;
  src.setRows({
      {1, "Java book", "X", "Y"},
  });

  readary::models::BookSearchProxyModel proxy;
  proxy.setSourceModel(&src);
  proxy.setSearchQuery("python");

  QCOMPARE(proxy.rowCount(), 0);

  // Full reset: Qt re-runs the filter over the new contents end-to-end.
  src.setRows({
      {10, "Python intro", "X", "Y"},
      {11, "Java again", "X", "Y"},
  });

  QCOMPARE(proxy.rowCount(), 1);
  QCOMPARE(idsOf(proxy), QList<int>({10}));
}

QTEST_GUILESS_MAIN(BookSearchProxyModelTest)
#include "BookSearchProxyModelTest.moc"
