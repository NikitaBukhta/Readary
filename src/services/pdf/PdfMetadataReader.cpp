#include "services/pdf/PdfMetadataReader.hpp"

#include "utils/IsbnValidator.hpp"

#include <QLoggingCategory>
#include <QPdfDocument>
#include <QPdfSelection>
#include <QSizeF>

#include <algorithm>

namespace {
Q_LOGGING_CATEGORY(lcPdf, "readary.services.pdf")

// Where a book actually prints its ISBN: the front matter's copyright page, and
// failing that the back matter. Bounded on purpose — a 2000-page manual would
// otherwise be read end to end for one number.
constexpr int g_frontPagesScanned = 12;
constexpr int g_backPagesScanned = 4;

qint64 findIsbnOnPages(QPdfDocument &document, int first, int last) {
  for (int page = first; page < last; ++page) {
    const QString text = document.getAllText(page).text();
    if (text.isEmpty()) {
      continue;
    }
    if (const auto isbn = readary::utils::IsbnValidator::findIn(text)) {
      return *isbn;
    }
  }
  return 0;
}

qint64 findIsbn(QPdfDocument &document) {
  const int pages = document.pageCount();

  const qint64 fromFront = findIsbnOnPages(document, 0, std::min(pages, g_frontPagesScanned));
  if (fromFront != 0) {
    return fromFront;
  }
  return findIsbnOnPages(document, std::max(0, pages - g_backPagesScanned), pages);
}

QString metaString(const QPdfDocument &document, QPdfDocument::MetaDataField field) {
  return document.metaData(field).toString().trimmed();
}

QImage renderCover(QPdfDocument &document, QSize coverSize) {
  const QSizeF pageSize = document.pagePointSize(0);
  if (pageSize.isEmpty()) {
    qCWarning(lcPdf) << "First page has no size, skipping the cover render";
    return {};
  }

  // Scaled rather than stretched to coverSize: a page is never the cover slot's
  // aspect ratio, and a squashed cover looks broken.
  const QSize target = pageSize.scaled(coverSize, Qt::KeepAspectRatio).toSize();
  if (target.isEmpty()) {
    return {};
  }
  return document.render(0, target);
}
} // namespace

namespace readary::services {

PdfDocumentInfo PdfMetadataReader::read(const QString &filePath, QSize coverSize) {
  QPdfDocument document;
  if (const QPdfDocument::Error error = document.load(filePath); error != QPdfDocument::Error::None) {
    qCWarning(lcPdf) << "Cannot read pdf:" << filePath << "error:" << static_cast<int>(error);
    return {};
  }

  if (document.pageCount() <= 0) {
    qCWarning(lcPdf) << "Pdf reports no pages:" << filePath;
    return {};
  }

  PdfDocumentInfo info;
  info.pageCount = document.pageCount();
  info.isbn = findIsbn(document);
  info.title = metaString(document, QPdfDocument::MetaDataField::Title);
  info.author = metaString(document, QPdfDocument::MetaDataField::Author);
  info.subject = metaString(document, QPdfDocument::MetaDataField::Subject);
  info.cover = renderCover(document, coverSize);

  qCInfo(lcPdf) << "Read pdf:" << filePath << "pages:" << info.pageCount << "title:" << info.title
                << "isbn:" << info.isbn << "cover:" << !info.cover.isNull();
  return info;
}

} // namespace readary::services
