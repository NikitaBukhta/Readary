#include "BookFileStore.hpp"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QUrl>

#include <utility>

using Qt::StringLiterals::operator""_L1;
using Qt::StringLiterals::operator""_s;

namespace {
Q_LOGGING_CATEGORY(lcFiles, "readary.services.files")

constexpr auto g_pdfDirName = "pdfs"_L1;
constexpr auto g_coverDirName = "covers"_L1;

// Enough of the digest to tell two renders apart without bloating the stored url.
constexpr qsizetype g_versionLength = 8;
} // namespace

namespace readary::services {

BookFileStore::BookFileStore(QString rootPath) : _rootPath{std::move(rootPath)} {}

QString BookFileStore::ensureSubdir(const QString &name) const {
  QString path = QDir{_rootPath}.absoluteFilePath(name);
  if (!QDir{}.mkpath(path)) {
    qCWarning(lcFiles) << "Cannot create directory:" << path;
    return {};
  }
  return path;
}

QString BookFileStore::pdfPath(qint64 isbn) const {
  return QDir{_rootPath}.absoluteFilePath(u"%1/%2.pdf"_s.arg(g_pdfDirName).arg(isbn));
}

QString BookFileStore::coverPath(qint64 isbn) const {
  return QDir{_rootPath}.absoluteFilePath(u"%1/%2.png"_s.arg(g_coverDirName).arg(isbn));
}

QString BookFileStore::storePdf(qint64 isbn, const QString &sourcePath) const {
  if (isbn <= 0 || sourcePath.isEmpty()) {
    return {};
  }

  if (ensureSubdir(g_pdfDirName).isEmpty()) {
    return {};
  }

  QString target = pdfPath(isbn);
  // The file dialog behind "Replace PDF" can navigate into the store itself, so
  // the pick may already be the stored file — removing it below would destroy
  // the very thing being stored. Existence is part of the test because QFileInfo
  // compares two missing paths as equal, both canonicalising to nothing.
  const QFileInfo sourceInfo{sourcePath};
  if (sourceInfo.exists() && sourceInfo == QFileInfo{target}) {
    return target;
  }

  // QFile::copy refuses an existing target, and replacing the book's PDF is a
  // supported action — drop the old one first.
  if (QFile::exists(target) && !QFile::remove(target)) {
    qCWarning(lcFiles) << "Cannot replace the stored pdf:" << target;
    return {};
  }

  if (!QFile::copy(sourcePath, target)) {
    qCWarning(lcFiles) << "Cannot copy pdf" << sourcePath << "to" << target;
    return {};
  }

  qCInfo(lcFiles) << "Stored pdf for book isbn:" << isbn;
  return target;
}

QString BookFileStore::storeCover(qint64 isbn, const QImage &cover) const {
  if (isbn <= 0 || cover.isNull()) {
    return {};
  }

  if (ensureSubdir(g_coverDirName).isEmpty()) {
    return {};
  }

  QString target = coverPath(isbn);
  if (!cover.save(target, "PNG")) {
    qCWarning(lcFiles) << "Cannot write cover:" << target;
    return {};
  }

  qCInfo(lcFiles) << "Stored cover for book isbn:" << isbn;
  return target;
}

QString BookFileStore::coverUrl(qint64 isbn) const {
  QFile file{coverPath(isbn)};
  if (!file.open(QIODevice::ReadOnly)) {
    return {};
  }

  const QByteArray version =
      QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha1).toHex().left(g_versionLength);
  QUrl url = QUrl::fromLocalFile(coverPath(isbn));
  url.setQuery(u"v="_s + QString::fromLatin1(version));
  return url.toString();
}

bool BookFileStore::removePdf(qint64 isbn) const {
  const QString target = pdfPath(isbn);
  if (!QFile::exists(target)) {
    // Nothing to do is the end state the caller asked for.
    return true;
  }

  if (!QFile::remove(target)) {
    qCWarning(lcFiles) << "Cannot remove the stored pdf:" << target;
    return false;
  }

  qCInfo(lcFiles) << "Removed pdf for book isbn:" << isbn;
  return true;
}

void BookFileStore::removeAll(qint64 isbn) const {
  removePdf(isbn);
  if (const QString cover = coverPath(isbn); QFile::exists(cover) && !QFile::remove(cover)) {
    qCWarning(lcFiles) << "Cannot remove the stored cover:" << cover;
  }
}

QString BookFileStore::toLocalPath(const QString &fileUrl) {
  const QUrl url{fileUrl};
  if (!url.isLocalFile()) {
    return fileUrl;
  }
  return url.toLocalFile();
}

} // namespace readary::services
