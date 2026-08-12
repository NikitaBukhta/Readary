#include "api/translate/GoogleTranslator.hpp"
#include "support/FakeHttpServer.hpp"

#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSignalSpy>
#include <QString>
#include <QTest>

using namespace Qt::StringLiterals;

using readary::api::GoogleTranslator;
using readary::tests::FakeHttpServer;

namespace {

QByteArray translationBody() {
  const QJsonArray segments{
      QJsonArray{u"Хорошая книга. "_s, u"A good book. "_s},
      QJsonArray{u"Очень."_s, u"Very."_s},
  };
  return QJsonDocument{QJsonArray{segments, QJsonValue{QJsonValue::Null}, u"en"_s}}.toJson(QJsonDocument::Compact);
}

QByteArray respond(const QUrl &target) {
  Q_UNUSED(target)
  return translationBody();
}

} // namespace

class GoogleTranslatorTest : public QObject {
  Q_OBJECT

private slots:
  void translate_joinsSegmentsAndEchoesRequestId();
  void translate_errorStillCompletesWithEmptyText();
  void translate_emptyInputCompletesWithoutRequest();
};

void GoogleTranslatorTest::translate_joinsSegmentsAndEchoesRequestId() {
  FakeHttpServer server{respond};
  QVERIFY(server.start());

  GoogleTranslator translator;
  translator.setEndpoint(server.endpoint(u"/translate_a/single"_s));

  QSignalSpy spy{&translator, &GoogleTranslator::translationReady};
  translator.translate(u"A good book. Very."_s, u"ru"_s, 7);
  QVERIFY(spy.wait(5000));

  QCOMPARE(spy.constFirst().at(0).toULongLong(), 7ULL);
  QCOMPARE(spy.constFirst().at(1).toString(), u"Хорошая книга. Очень."_s);
  QCOMPARE(server.lastQueryItem(u"q"_s), u"A good book. Very."_s);
  QCOMPARE(server.lastQueryItem(u"tl"_s), u"ru"_s);
}

void GoogleTranslatorTest::translate_errorStillCompletesWithEmptyText() {
  FakeHttpServer server{respond};
  QVERIFY(server.start());
  // The real endpoint answers 403 to a request without a browser-like agent.
  server.setFailureStatus("403 Forbidden");
  server.setFailRequests(true);

  GoogleTranslator translator;
  translator.setEndpoint(server.endpoint(u"/translate_a/single"_s));

  QSignalSpy spy{&translator, &GoogleTranslator::translationReady};
  translator.translate(u"A good book."_s, u"ru"_s, 3);

  QVERIFY(spy.wait(5000));
  QCOMPARE(spy.constFirst().at(0).toULongLong(), 3ULL);
  QCOMPARE(spy.constFirst().at(1).toString(), QString{});
}

void GoogleTranslatorTest::translate_emptyInputCompletesWithoutRequest() {
  FakeHttpServer server{respond};
  QVERIFY(server.start());

  GoogleTranslator translator;
  translator.setEndpoint(server.endpoint(u"/translate_a/single"_s));

  QSignalSpy spy{&translator, &GoogleTranslator::translationReady};
  translator.translate(QString{}, u"ru"_s, 11);

  QCOMPARE(spy.size(), 1); // answered inline, no round trip
  QCOMPARE(spy.constFirst().at(0).toULongLong(), 11ULL);
  QCOMPARE(spy.constFirst().at(1).toString(), QString{});
  QCOMPARE(server.requestCount(), 0);
}

QTEST_GUILESS_MAIN(GoogleTranslatorTest)
#include "GoogleTranslatorTest.moc"
