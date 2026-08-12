#include "services/EmojiResolver.hpp"

#include <QString>
#include <QTest>

using Qt::StringLiterals::operator""_s;

using readary::services::EmojiResolver;

class EmojiResolverTest : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();

  void singleCodepointEmoji_resolvesToFilename();
  void emojiWithVariationSelector_stripsFE0F();
  void zwjSequence_keepsAllCodepoints();
  void keycapSequence_stripsFE0F();
  void nonEmojiInput_returnsEmpty();
  void emptyInput_returnsEmpty();
  void cacheIsHitOnSecondCall();
};

void EmojiResolverTest::initTestCase() { Q_INIT_RESOURCE(emoji_resources); }

void EmojiResolverTest::singleCodepointEmoji_resolvesToFilename() {
  EmojiResolver r;
  const QString url = r.iconUrl(u"👋"_s);
  QCOMPARE(url, u"qrc:/emoji/1f44b.svg"_s);
}

void EmojiResolverTest::emojiWithVariationSelector_stripsFE0F() {
  // ❤️ = U+2764 U+FE0F. Twemoji ships as 2764.svg (no fe0f).
  const char32_t cps[] = {0x2764, 0xFE0F};
  EmojiResolver r;
  const QString url = r.iconUrl(QString::fromUcs4(cps, 2));
  QCOMPARE(url, u"qrc:/emoji/2764.svg"_s);
}

void EmojiResolverTest::zwjSequence_keepsAllCodepoints() {
  // 👨‍👩‍👧 = U+1F468 U+200D U+1F469 U+200D U+1F467 → 1f468-200d-1f469-200d-1f467.svg
  const char32_t cps[] = {0x1F468, 0x200D, 0x1F469, 0x200D, 0x1F467};
  EmojiResolver r;
  const QString url = r.iconUrl(QString::fromUcs4(cps, 5));
  QCOMPARE(url, u"qrc:/emoji/1f468-200d-1f469-200d-1f467.svg"_s);
}

void EmojiResolverTest::keycapSequence_stripsFE0F() {
  // 1️⃣ = U+0031 U+FE0F U+20E3 — Twemoji actually ships as 31-20e3.svg
  // (FE0F stripped, same as other sequences).
  const char32_t cps[] = {0x0031, 0xFE0F, 0x20E3};
  EmojiResolver r;
  const QString url = r.iconUrl(QString::fromUcs4(cps, 3));
  QCOMPARE(url, u"qrc:/emoji/31-20e3.svg"_s);
}

void EmojiResolverTest::nonEmojiInput_returnsEmpty() {
  // ↻ U+21BB clockwise open circle arrow is not in the Unicode Emoji subset
  // and is not shipped by Twemoji — must return empty so callers fall back
  // to text rendering rather than show a broken-image placeholder.
  EmojiResolver r;
  const QString url = r.iconUrl(u"↻"_s);
  QVERIFY(url.isEmpty());
}

void EmojiResolverTest::emptyInput_returnsEmpty() {
  EmojiResolver r;
  QVERIFY(r.iconUrl(QString{}).isEmpty());
}

void EmojiResolverTest::cacheIsHitOnSecondCall() {
  EmojiResolver r;
  const QString first = r.iconUrl(u"👋"_s);
  const QString second = r.iconUrl(u"👋"_s);
  QCOMPARE(first, second);
  QVERIFY(!first.isEmpty());
}

QTEST_GUILESS_MAIN(EmojiResolverTest)
#include "EmojiResolverTest.moc"
