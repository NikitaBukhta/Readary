#include "qmltypes/BookStatisticsObject.hpp"
#include "services/dto/BookStatisticsDTO.hpp"
#include "support/GadgetAccess.hpp"

#include <QTest>
#include <QVariantMap>

using readary::qmltypes::BookStatisticsObject;
using readary::services::BookStatisticsDTO;
using readary::tests::readGadget;

namespace {

// Every scalar carries a different value on purpose: a wrapper that crossed
// two properties over would still pass against realistic-looking data where
// several fields share a number.
BookStatisticsDTO makeStatistics() {
  BookStatisticsDTO stats;
  stats.sessionCount = 1;
  stats.timedSessionCount = 2;
  stats.pagesRead = 3;
  stats.totalSeconds = 4;
  stats.averagePagesPerHour = 5.0;
  stats.minPagesPerHour = 6.0;
  stats.maxPagesPerHour = 7.0;
  stats.weeklyPages = {11, 12, 13, 14, 15, 16, 17};
  stats.progressPoints = {{.session = 1, .page = 30}, {.session = 2, .page = 50}, {.session = 3, .page = 110}};
  return stats;
}

} // namespace

// The Q_GADGET wrapper is what the statistics page sees; the DTO underneath
// stays free of moc. These tests go through the metaobject, the way QML does.
class BookStatisticsObjectTest : public QObject {
  Q_OBJECT

private slots:
  void defaultConstructed_isAnEmptyButSevenDayWeek();
  void constructedFromADto_copiesEveryField();
  void scalars_readThroughTheMetaObject_data();
  void scalars_readThroughTheMetaObject();
  void weeklyPages_crossAsAnIntSequence();
  void progressPoints_crossAsSessionAndPageObjects();
  void unknownProperty_isNotFound();
};

void BookStatisticsObjectTest::defaultConstructed_isAnEmptyButSevenDayWeek() {
  const BookStatisticsObject stats;

  QCOMPARE(stats.sessionCount, 0);
  QCOMPARE(stats.weeklyPages.size(), BookStatisticsDTO::kDaysInWeek);
  QVERIFY(stats.progressPoints.isEmpty());
}

void BookStatisticsObjectTest::constructedFromADto_copiesEveryField() {
  const BookStatisticsDTO source = makeStatistics();
  const BookStatisticsObject stats{source};

  QCOMPARE(stats.pagesRead, source.pagesRead);
  QCOMPARE(stats.weeklyPages, source.weeklyPages);
  QCOMPARE(stats.progressPoints.size(), source.progressPoints.size());
}

void BookStatisticsObjectTest::scalars_readThroughTheMetaObject_data() {
  QTest::addColumn<QByteArray>("property");
  QTest::addColumn<double>("expected");

  QTest::newRow("sessionCount") << QByteArray("sessionCount") << 1.0;
  QTest::newRow("timedSessionCount") << QByteArray("timedSessionCount") << 2.0;
  QTest::newRow("pagesRead") << QByteArray("pagesRead") << 3.0;
  QTest::newRow("totalSeconds") << QByteArray("totalSeconds") << 4.0;
  QTest::newRow("averagePagesPerHour") << QByteArray("averagePagesPerHour") << 5.0;
  QTest::newRow("minPagesPerHour") << QByteArray("minPagesPerHour") << 6.0;
  QTest::newRow("maxPagesPerHour") << QByteArray("maxPagesPerHour") << 7.0;
}

void BookStatisticsObjectTest::scalars_readThroughTheMetaObject() {
  QFETCH(QByteArray, property);
  QFETCH(double, expected);

  const BookStatisticsObject stats{makeStatistics()};

  const QVariant value = readGadget(stats, property.constData());
  QVERIFY2(value.isValid(), property.constData());
  QCOMPARE(value.toDouble(), expected);
}

void BookStatisticsObjectTest::weeklyPages_crossAsAnIntSequence() {
  const BookStatisticsObject stats{makeStatistics()};

  const QVariant weekly = readGadget(stats, "weeklyPages");
  QCOMPARE(weekly.value<QList<int>>(), QList<int>({11, 12, 13, 14, 15, 16, 17}));
}

void BookStatisticsObjectTest::progressPoints_crossAsSessionAndPageObjects() {
  // The chart walks `point.session` / `point.page` in QML, so the keys are
  // part of the contract.
  const BookStatisticsObject stats{makeStatistics()};

  const QVariantList points = readGadget(stats, "progressPoints").toList();
  QCOMPARE(points.size(), 3);
  QCOMPARE(points.at(0).toMap().value(QStringLiteral("session")).toInt(), 1);
  QCOMPARE(points.at(0).toMap().value(QStringLiteral("page")).toInt(), 30);
  QCOMPARE(points.at(2).toMap().value(QStringLiteral("page")).toInt(), 110);
}

void BookStatisticsObjectTest::unknownProperty_isNotFound() {
  QCOMPARE(BookStatisticsObject::staticMetaObject.indexOfProperty("booksPerMonth"), -1);
}

QTEST_GUILESS_MAIN(BookStatisticsObjectTest)
#include "BookStatisticsObjectTest.moc"
