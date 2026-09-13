#ifndef READARY_SERVICES_PDFDOCUMENTINFO_HPP
#define READARY_SERVICES_PDFDOCUMENTINFO_HPP

#include <QImage>
#include <QString>

namespace readary::services {

// What a PDF can tell us about the book it holds. Every field is best-effort:
// the page count always comes back, while `/Info` metadata is frequently empty
// or authoring-tool noise, and the cover is a render of page one.
struct PdfDocumentInfo {
  int pageCount = 0;
  // Normalised to ISBN-13 by IsbnValidator, 0 when the book does not print one
  // where we look (or prints it as an image, as a pure scan does).
  qint64 isbn = 0;
  QString title;
  QString author;
  QString subject;
  QImage cover;

  bool isValid() const { return pageCount > 0; }
};

} // namespace readary::services

#endif // READARY_SERVICES_PDFDOCUMENTINFO_HPP
