#ifndef READARY_TESTS_SUPPORT_MINIMALPDF_HPP
#define READARY_TESTS_SUPPORT_MINIMALPDF_HPP

#include <QByteArray>
#include <QFile>
#include <QList>
#include <QString>

namespace readary::tests {

// A valid uncompressed PDF built byte by byte: real xref offsets and trailer,
// so PDFium parses it as a well-formed document rather than falling back on its
// damaged-file repair path. Generated rather than shipped as a binary fixture
// so the page count and /Info fields under test are visible in the test itself.
namespace pdf {

inline QByteArray escaped(const QString &text) {
  QByteArray out;
  for (const char ch : text.toUtf8()) {
    if (ch == '(' || ch == ')' || ch == '\\') {
      out += '\\';
    }
    out += ch;
  }
  return out;
}

inline QByteArray makeDocument(int pageCount, const QString &title = {}, const QString &author = {},
                               const QString &subject = {}, const QString &printedIsbn = {}) {
  QByteArray pdf = "%PDF-1.4\n";
  QList<qsizetype> offsets;

  const auto addObject = [&pdf, &offsets](const QByteArray &content) {
    offsets.append(pdf.size());
    pdf += QByteArray::number(offsets.size()) + " 0 obj\n" + content + "\nendobj\n";
  };

  // 1 = catalog, 2 = page tree, 3 = one content stream shared by every page,
  // 4..3+n = the pages, 4+n = the info dictionary.
  const int firstPageObject = 4;
  QByteArray kids;
  for (int i = 0; i < pageCount; ++i) {
    kids += QByteArray::number(firstPageObject + i) + " 0 R ";
  }

  addObject("<< /Type /Catalog /Pages 2 0 R >>");
  addObject("<< /Type /Pages /Kids [" + kids.trimmed() + "] /Count " + QByteArray::number(pageCount) + " >>");

  // Text needs a font, so the resource dictionary below is only filled in when
  // the caller wants an ISBN printed on the page for the scanner to find.
  QByteArray stream = "0 0 1 RG 4 w 20 20 160 260 re S\n";
  if (!printedIsbn.isEmpty()) {
    stream += "BT /F1 12 Tf 24 240 Td (ISBN " + escaped(printedIsbn) + ") Tj ET\n";
  }
  addObject("<< /Length " + QByteArray::number(stream.size()) + " >>\nstream\n" + stream + "endstream");

  const QByteArray resources = printedIsbn.isEmpty() ? QByteArray{"<< >>"}
                                                     : QByteArray{"<< /Font << /F1 << /Type /Font /Subtype /Type1 "
                                                                  "/BaseFont /Helvetica >> >> >>"};
  for (int i = 0; i < pageCount; ++i) {
    addObject("<< /Type /Page /Parent 2 0 R /MediaBox [0 0 200 300] /Resources " + resources + " /Contents 3 0 R >>");
  }

  QByteArray info = "<<";
  if (!title.isEmpty()) {
    info += " /Title (" + escaped(title) + ")";
  }
  if (!author.isEmpty()) {
    info += " /Author (" + escaped(author) + ")";
  }
  if (!subject.isEmpty()) {
    info += " /Subject (" + escaped(subject) + ")";
  }
  info += " >>";
  addObject(info);
  const qsizetype infoObject = offsets.size();

  // Every xref entry is exactly 20 bytes wide — PDFium relies on that to seek
  // within the table.
  const qsizetype xrefOffset = pdf.size();
  pdf += "xref\n0 " + QByteArray::number(offsets.size() + 1) + "\n";
  pdf += "0000000000 65535 f \n";
  for (const qsizetype offset : offsets) {
    pdf += QByteArray::number(offset).rightJustified(10, '0') + " 00000 n \n";
  }

  pdf += "trailer\n<< /Size " + QByteArray::number(offsets.size() + 1) + " /Root 1 0 R /Info " +
         QByteArray::number(infoObject) + " 0 R >>\n";
  pdf += "startxref\n" + QByteArray::number(xrefOffset) + "\n%%EOF\n";
  return pdf;
}

inline bool writeDocument(const QString &path, int pageCount, const QString &title = {}, const QString &author = {},
                          const QString &subject = {}, const QString &printedIsbn = {}) {
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return false;
  }
  const QByteArray document = makeDocument(pageCount, title, author, subject, printedIsbn);
  return file.write(document) == document.size();
}

} // namespace pdf
} // namespace readary::tests

#endif // READARY_TESTS_SUPPORT_MINIMALPDF_HPP
