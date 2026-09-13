#ifndef READARY_SERVICES_PDFMETADATAREADER_HPP
#define READARY_SERVICES_PDFMETADATAREADER_HPP

#include "PdfDocumentInfo.hpp"

#include <QSize>
#include <QString>

namespace readary::services {

// Reads what a book's PDF knows about itself through Qt PDF (PDFium).
// Stateless: one call, one document, no ownership.
class PdfMetadataReader {
public:
  // Page one rendered at this size at most, aspect preserved. Sized for the
  // detail page's cover at 3x so it still looks sharp on a dense screen.
  static constexpr QSize kDefaultCoverSize{300, 420};

  static PdfDocumentInfo read(const QString &filePath, QSize coverSize = kDefaultCoverSize);
};

} // namespace readary::services

#endif // READARY_SERVICES_PDFMETADATAREADER_HPP
