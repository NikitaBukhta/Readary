#ifndef READARY_SERVICES_BOOKFILESTORE_HPP
#define READARY_SERVICES_BOOKFILESTORE_HPP

#include <QImage>
#include <QString>

namespace readary::services {

// Owns the on-disk files that belong to a book — the attached PDF and the cover
// rendered from it — under one root the caller supplies (the app data dir in the
// running app, a temp dir in tests).
//
// Files are copied in rather than referenced in place, so the library keeps
// working after the user moves or deletes the original. Everything is named
// after the book's isbn, which makes cleanup on delete a lookup, not a search.
class BookFileStore {
public:
  explicit BookFileStore(QString rootPath);

  // Both return the stored path, or an empty string when nothing was written.
  QString storePdf(qint64 isbn, const QString &sourcePath) const;
  QString storeCover(qint64 isbn, const QImage &cover) const;

  bool removePdf(qint64 isbn) const;
  // Drops every file for the book. Used when the book itself goes away.
  void removeAll(qint64 isbn) const;

  QString pdfPath(qint64 isbn) const;
  QString coverPath(qint64 isbn) const;

  // The cover's file url for the books row, or empty when there is no file.
  // Carries a hash of the file's contents as a query: Image caches by url and
  // the cover keeps a stable file name, so a re-rendered cover at the same url
  // would keep showing the old page. QUrl::toLocalFile() drops the query, so the
  // file still opens, and identical content still yields an identical url.
  QString coverUrl(qint64 isbn) const;

  // QML hands over a URL, QFile wants a path. On Android a picked file arrives
  // as `content://…`, which has no local path — Qt's file engine opens that
  // scheme directly, so the raw string is passed through untouched.
  static QString toLocalPath(const QString &fileUrl);

private:
  QString ensureSubdir(const QString &name) const;

  QString _rootPath;
};

} // namespace readary::services

#endif // READARY_SERVICES_BOOKFILESTORE_HPP
