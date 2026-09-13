#include "services/dto/CharacterDTO.hpp"

#include <QTest>

using Qt::StringLiterals::operator""_s;

using readary::services::CharacterDTO;

class CharacterDTOTest : public QObject {
  Q_OBJECT

private slots:
  void fromMap_readsEveryColumn();
  void fromMap_missingRole_leavesItEmpty();
  void fromMap_emptyMap_isADefaultCharacter();
  void fromMap_coercesTheIdFromText();
};

void CharacterDTOTest::fromMap_readsEveryColumn() {
  const CharacterDTO character = CharacterDTO::fromMap({
      {u"id"_s, 7LL},
      {u"name"_s, u"Paul Atreides"_s},
      {u"role"_s, u"Protagonist"_s},
  });

  QCOMPARE(character.id, 7LL);
  QCOMPARE(character.name, u"Paul Atreides"_s);
  QCOMPARE(character.role, u"Protagonist"_s);
}

void CharacterDTOTest::fromMap_missingRole_leavesItEmpty() {
  // role is nullable in the schema.
  const CharacterDTO character = CharacterDTO::fromMap({{u"id"_s, 1LL}, {u"name"_s, u"Chani"_s}});

  QCOMPARE(character.name, u"Chani"_s);
  QVERIFY(character.role.isEmpty());
}

void CharacterDTOTest::fromMap_emptyMap_isADefaultCharacter() {
  const CharacterDTO character = CharacterDTO::fromMap({});

  QCOMPARE(character.id, 0LL);
  QVERIFY(character.name.isEmpty());
  QVERIFY(character.role.isEmpty());
}

void CharacterDTOTest::fromMap_coercesTheIdFromText() {
  const CharacterDTO character = CharacterDTO::fromMap({{u"id"_s, u"42"_s}});
  QCOMPARE(character.id, 42LL);
}

QTEST_GUILESS_MAIN(CharacterDTOTest)
#include "CharacterDTOTest.moc"
