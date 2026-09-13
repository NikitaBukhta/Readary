#include "services/PdfMetadataReader.hpp"
#include "support/MinimalPdf.hpp"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

using Qt::StringLiterals::operator""_s;

using readary::services::PdfDocumentInfo;
using readary::services::PdfMetadataReader;

class PdfMetadataReaderTest : public QObject {
  Q_OBJECT

private slots:
  void init();
  void cleanup();

  void read_reportsThePageCount();
  void read_readsTheInfoDictionary();
  void read_withoutMetadata_leavesTheStringsEmpty();
  void read_rendersTheFirstPageAsACover();
  void read_coverKeepsThePageAspectRatio();
  void read_coverFitsInsideTheRequestedSize();
  void read_findsTheIsbnPrintedOnThePage();
  void read_withoutAPrintedIsbn_reportsNone();

  void read_missingFile_isInvalid();
  void read_fileThatIsNotAPdf_isInvalid();
  void read_emptyPath_isInvalid();

private:
  QString pathFor(const QString &name) const;
  QString writePdf(const QString &name, int pageCount, const QString &title = {}, const QString &author = {},
                   const QString &subject = {}, const QString &printedIsbn = {}) const;

  QTemporaryDir _dir;
};

void PdfMetadataReaderTest::init() { QVERIFY(_dir.isValid()); }

void PdfMetadataReaderTest::cleanup() {}

QString PdfMetadataReaderTest::pathFor(const QString &name) const { return QDir{_dir.path()}.absoluteFilePath(name); }

QString PdfMetadataReaderTest::writePdf(const QString &name, int pageCount, const QString &title, const QString &author,
                                        const QString &subject, const QString &printedIsbn) const {
  QString path = pathFor(name);
  const bool written = readary::tests::pdf::writeDocument(path, pageCount, title, author, subject, printedIsbn);
  return written ? path : QString{};
}

void PdfMetadataReaderTest::read_reportsThePageCount() {
  const QString path = writePdf(u"three.pdf"_s, 3);
  QVERIFY(!path.isEmpty());

  const PdfDocumentInfo info = PdfMetadataReader::read(path);

  QVERIFY(info.isValid());
  QCOMPARE(info.pageCount, 3);
}

void PdfMetadataReaderTest::read_readsTheInfoDictionary() {
  const QString path = writePdf(u"meta.pdf"_s, 1, u"Refactoring"_s, u"Martin Fowler"_s, u"Improving the design"_s);
  QVERIFY(!path.isEmpty());

  const PdfDocumentInfo info = PdfMetadataReader::read(path);

  QCOMPARE(info.title, u"Refactoring"_s);
  QCOMPARE(info.author, u"Martin Fowler"_s);
  QCOMPARE(info.subject, u"Improving the design"_s);
}

void PdfMetadataReaderTest::read_withoutMetadata_leavesTheStringsEmpty() {
  // An empty /Info dictionary is common, and the caller distinguishes "the pdf
  // says nothing" from "the pdf says this" to decide whether to override.
  const QString path = writePdf(u"bare.pdf"_s, 2);
  QVERIFY(!path.isEmpty());

  const PdfDocumentInfo info = PdfMetadataReader::read(path);

  QVERIFY(info.isValid());
  QVERIFY(info.title.isEmpty());
  QVERIFY(info.author.isEmpty());
  QVERIFY(info.subject.isEmpty());
}

void PdfMetadataReaderTest::read_rendersTheFirstPageAsACover() {
  const QString path = writePdf(u"cover.pdf"_s, 1);
  QVERIFY(!path.isEmpty());

  const PdfDocumentInfo info = PdfMetadataReader::read(path);

  QVERIFY(!info.cover.isNull());
  QVERIFY(info.cover.width() > 0);
  QVERIFY(info.cover.height() > 0);
}

void PdfMetadataReaderTest::read_coverKeepsThePageAspectRatio() {
  // The fixture page is 200x300pt; a stretched cover would look broken in the
  // cover slot, so the render must not take the slot's ratio.
  const QString path = writePdf(u"aspect.pdf"_s, 1);
  QVERIFY(!path.isEmpty());

  const PdfDocumentInfo info = PdfMetadataReader::read(path, QSize{600, 600});

  QVERIFY(!info.cover.isNull());
  const double ratio = static_cast<double>(info.cover.width()) / info.cover.height();
  QVERIFY2(std::abs(ratio - (200.0 / 300.0)) < 0.02, qPrintable(u"ratio was %1"_s.arg(ratio)));
}

void PdfMetadataReaderTest::read_coverFitsInsideTheRequestedSize() {
  const QString path = writePdf(u"bounded.pdf"_s, 1);
  QVERIFY(!path.isEmpty());

  const PdfDocumentInfo info = PdfMetadataReader::read(path, QSize{80, 80});

  QVERIFY(!info.cover.isNull());
  QVERIFY(info.cover.width() <= 80);
  QVERIFY(info.cover.height() <= 80);
}

void PdfMetadataReaderTest::read_findsTheIsbnPrintedOnThePage() {
  // No pdf metadata field carries an ISBN, so it is read off the page the way a
  // person would read it.
  const QString path = writePdf(u"isbn.pdf"_s, 1, {}, {}, {}, u"978-0-13-235088-4"_s);
  QVERIFY(!path.isEmpty());

  const PdfDocumentInfo info = PdfMetadataReader::read(path);

  QCOMPARE(info.isbn, 9780132350884LL);
}

void PdfMetadataReaderTest::read_withoutAPrintedIsbn_reportsNone() {
  const QString path = writePdf(u"noisbn.pdf"_s, 2);
  QVERIFY(!path.isEmpty());

  QCOMPARE(PdfMetadataReader::read(path).isbn, 0LL);
}

void PdfMetadataReaderTest::read_missingFile_isInvalid() {
  const PdfDocumentInfo info = PdfMetadataReader::read(pathFor(u"nope.pdf"_s));

  QVERIFY(!info.isValid());
  QCOMPARE(info.pageCount, 0);
  QVERIFY(info.cover.isNull());
}

void PdfMetadataReaderTest::read_fileThatIsNotAPdf_isInvalid() {
  const QString path = pathFor(u"notes.txt"_s);
  QFile file(path);
  QVERIFY(file.open(QIODevice::WriteOnly));
  QVERIFY(file.write("this is not a pdf at all") > 0);
  file.close();

  QVERIFY(!PdfMetadataReader::read(path).isValid());
}

void PdfMetadataReaderTest::read_emptyPath_isInvalid() { QVERIFY(!PdfMetadataReader::read({}).isValid()); }

QTEST_GUILESS_MAIN(PdfMetadataReaderTest)
#include "PdfMetadataReaderTest.moc"
