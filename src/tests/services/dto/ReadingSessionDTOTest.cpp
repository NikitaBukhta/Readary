#include "services/dto/ReadingSessionDTO.hpp"

#include <QDateTime>
#include <QTest>
#include <QTimeZone>

using Qt::StringLiterals::operator""_s;

using readary::services::ReadingSessionDTO;

namespace {

QDateTime utc(int year, int month, int day, int hour, int minute) {
  return QDateTime{QDate{year, month, day}, QTime{hour, minute}, QTimeZone::UTC};
}

} // namespace

class ReadingSessionDTOTest : public QObject {
  Q_OBJECT

private slots:
  void fromMap_readsEveryColumn();
  void fromMap_parsesIsoTextStamps();
  void fromMap_acceptsDateTimesFromTheDriver();
  void fromMap_normalizesStampsToLocalTime();
  void fromMap_zonelessStamp_isReadAsLocalTime();
  void fromMap_openSession_hasNoEndStamp();
  void fromMap_unparsableStamp_isInvalid();
  void fromMap_emptyMap_isADefaultSession();

  void pagesRead_isTheRangeLength();
  void pagesRead_neverGoesNegative();
  void durationSeconds_isTheStampDistance();
  void durationSeconds_withoutBothStamps_isZero();
  void durationSeconds_neverGoesNegative();
};

void ReadingSessionDTOTest::fromMap_readsEveryColumn() {
  const ReadingSessionDTO session = ReadingSessionDTO::fromMap({
      {u"id"_s, 12LL},
      {u"book_isbn"_s, 9780201616224LL},
      {u"started_at"_s, u"2026-03-05T18:30:00Z"_s},
      {u"ended_at"_s, u"2026-03-05T20:00:00Z"_s},
      {u"pages_from"_s, 90},
      {u"pages_to"_s, 180},
  });

  QCOMPARE(session.id, 12LL);
  QCOMPARE(session.bookIsbn, 9780201616224LL);
  QCOMPARE(session.pagesFrom, 90);
  QCOMPARE(session.pagesTo, 180);
  QVERIFY(session.startedAt.isValid());
  QVERIFY(session.endedAt.isValid());
}

void ReadingSessionDTOTest::fromMap_parsesIsoTextStamps() {
  // The driver hands TEXT columns back as strings unless they were declared
  // with a date type, which reading_sessions is not.
  const ReadingSessionDTO session = ReadingSessionDTO::fromMap({{u"started_at"_s, u"2026-03-05T18:30:00Z"_s}});

  QCOMPARE(session.startedAt.toUTC(), utc(2026, 3, 5, 18, 30));
}

void ReadingSessionDTOTest::fromMap_acceptsDateTimesFromTheDriver() {
  const ReadingSessionDTO session = ReadingSessionDTO::fromMap({{u"started_at"_s, utc(2026, 3, 5, 18, 30)}});

  QCOMPARE(session.startedAt.toUTC(), utc(2026, 3, 5, 18, 30));
}

void ReadingSessionDTOTest::fromMap_normalizesStampsToLocalTime() {
  // QML formats what the reader's clock shows, so the DTO hands out local time.
  const ReadingSessionDTO session = ReadingSessionDTO::fromMap({
      {u"started_at"_s, u"2026-03-05T18:30:00Z"_s},
      {u"ended_at"_s, utc(2026, 3, 5, 20, 0)},
  });

  QCOMPARE(session.startedAt.timeRepresentation(), QTimeZone(QTimeZone::LocalTime));
  QCOMPARE(session.endedAt.timeRepresentation(), QTimeZone(QTimeZone::LocalTime));
}

void ReadingSessionDTOTest::fromMap_zonelessStamp_isReadAsLocalTime() {
  const ReadingSessionDTO session = ReadingSessionDTO::fromMap({{u"started_at"_s, u"2026-03-05T18:30:00"_s}});

  QCOMPARE(session.startedAt, QDateTime(QDate{2026, 3, 5}, QTime{18, 30}));
}

void ReadingSessionDTOTest::fromMap_openSession_hasNoEndStamp() {
  const ReadingSessionDTO session = ReadingSessionDTO::fromMap({
      {u"started_at"_s, u"2026-03-05T18:30:00Z"_s},
      {u"ended_at"_s, QVariant{}},
  });

  QVERIFY(session.startedAt.isValid());
  QVERIFY(!session.endedAt.isValid());
}

void ReadingSessionDTOTest::fromMap_unparsableStamp_isInvalid() {
  const ReadingSessionDTO session = ReadingSessionDTO::fromMap({{u"started_at"_s, u"yesterday evening"_s}});
  QVERIFY(!session.startedAt.isValid());
}

void ReadingSessionDTOTest::fromMap_emptyMap_isADefaultSession() {
  const ReadingSessionDTO session = ReadingSessionDTO::fromMap({});

  QCOMPARE(session.id, 0LL);
  QCOMPARE(session.pagesFrom, 0);
  QCOMPARE(session.pagesTo, 0);
  QVERIFY(!session.startedAt.isValid());
  QVERIFY(!session.endedAt.isValid());
  QCOMPARE(session.pagesRead(), 0);
  QCOMPARE(session.durationSeconds(), 0);
}

void ReadingSessionDTOTest::pagesRead_isTheRangeLength() {
  ReadingSessionDTO session;
  session.pagesFrom = 90;
  session.pagesTo = 180;

  QCOMPARE(session.pagesRead(), 90);
}

void ReadingSessionDTOTest::pagesRead_neverGoesNegative() {
  // The schema forbids it, but a hand-edited row must not print "-20 pages".
  ReadingSessionDTO session;
  session.pagesFrom = 100;
  session.pagesTo = 80;

  QCOMPARE(session.pagesRead(), 0);
}

void ReadingSessionDTOTest::durationSeconds_isTheStampDistance() {
  ReadingSessionDTO session;
  session.startedAt = utc(2026, 3, 5, 18, 30);
  session.endedAt = utc(2026, 3, 5, 20, 0);

  QCOMPARE(session.durationSeconds(), 90 * 60);
}

void ReadingSessionDTOTest::durationSeconds_withoutBothStamps_isZero() {
  ReadingSessionDTO session;
  session.startedAt = utc(2026, 3, 5, 18, 30);

  QCOMPARE(session.durationSeconds(), 0);

  session.startedAt = {};
  session.endedAt = utc(2026, 3, 5, 20, 0);
  QCOMPARE(session.durationSeconds(), 0);
}

void ReadingSessionDTOTest::durationSeconds_neverGoesNegative() {
  ReadingSessionDTO session;
  session.startedAt = utc(2026, 3, 5, 20, 0);
  session.endedAt = utc(2026, 3, 5, 18, 30);

  QCOMPARE(session.durationSeconds(), 0);
}

QTEST_GUILESS_MAIN(ReadingSessionDTOTest)
#include "ReadingSessionDTOTest.moc"
