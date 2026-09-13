#include "services/storage/BookFileStore.hpp"

#include <QDir>
#include <QFile>
#include <QImage>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

#include <memory>

using Qt::StringLiterals::operator""_s;

using readary::services::BookFileStore;

namespace {

constexpr qint64 kIsbn = 9780201616224LL;
constexpr qint64 kOtherIsbn = 9781491903995LL;

} // namespace

class BookFileStoreTest : public QObject {
  Q_OBJECT

private slots:
  void init();
  void cleanup();

  void storePdf_copiesTheFileIn();
  void storePdf_leavesTheOriginalAlone();
  void storePdf_survivesTheOriginalBeingDeleted();
  void storePdf_again_replacesTheStoredOne();
  void storePdf_missingSource_returnsNothing();
  void storePdf_withoutAnIsbn_returnsNothing();
  void storePdf_namesTheFileAfterTheBook();
  void storePdf_reStoringTheStoredFile_keepsIt();

  void storeCover_writesAPng();
  void storeCover_nullImage_returnsNothing();

  void coverUrl_pointsAtTheStoredFile();
  void coverUrl_changesWhenTheCoverDoes();
  void coverUrl_sameCover_staysTheSame();
  void coverUrl_withoutAFile_isEmpty();

  void removePdf_deletesIt();
  void removePdf_whenThereIsNone_stillReportsSuccess();
  void removePdf_leavesTheCoverBehind();
  void removeAll_dropsBothFiles();
  void removeAll_leavesOtherBooksAlone();

  void toLocalPath_convertsAFileUrl();
  void toLocalPath_passesANonLocalUrlThrough();

private:
  QString writeSourcePdf(const QString &name, const QByteArray &content = "%PDF-1.4 fake") const;

  // Rebuilt per test: files are named after an isbn, so one shared root would
  // let a file written by one test function decide the next one's result.
  std::unique_ptr<QTemporaryDir> _root;
  std::unique_ptr<QTemporaryDir> _incoming;
  std::unique_ptr<BookFileStore> _store;
};

void BookFileStoreTest::init() {
  _root = std::make_unique<QTemporaryDir>();
  _incoming = std::make_unique<QTemporaryDir>();
  QVERIFY(_root->isValid());
  QVERIFY(_incoming->isValid());
  _store = std::make_unique<BookFileStore>(_root->path());
}

void BookFileStoreTest::cleanup() {
  _store.reset();
  _incoming.reset();
  _root.reset();
}

QString BookFileStoreTest::writeSourcePdf(const QString &name, const QByteArray &content) const {
  QString path = QDir{_incoming->path()}.absoluteFilePath(name);
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) || file.write(content) != content.size()) {
    return {};
  }
  return path;
}

void BookFileStoreTest::storePdf_copiesTheFileIn() {
  const QString source = writeSourcePdf(u"book.pdf"_s);
  QVERIFY(!source.isEmpty());

  const QString stored = _store->storePdf(kIsbn, source);

  QVERIFY(!stored.isEmpty());
  QVERIFY(QFile::exists(stored));
  QCOMPARE(stored, _store->pdfPath(kIsbn));
}

void BookFileStoreTest::storePdf_leavesTheOriginalAlone() {
  const QString source = writeSourcePdf(u"book.pdf"_s);
  QVERIFY(!source.isEmpty());

  QVERIFY(!_store->storePdf(kIsbn, source).isEmpty());

  QVERIFY(QFile::exists(source));
}

void BookFileStoreTest::storePdf_survivesTheOriginalBeingDeleted() {
  // The whole point of copying rather than referencing: moving or deleting the
  // picked file must not break the book.
  const QString source = writeSourcePdf(u"book.pdf"_s, "%PDF-1.4 original");
  QVERIFY(!source.isEmpty());
  const QString stored = _store->storePdf(kIsbn, source);
  QVERIFY(!stored.isEmpty());

  QVERIFY(QFile::remove(source));

  QVERIFY(QFile::exists(stored));
  QFile file(stored);
  QVERIFY(file.open(QIODevice::ReadOnly));
  QCOMPARE(file.readAll(), QByteArray{"%PDF-1.4 original"});
}

void BookFileStoreTest::storePdf_again_replacesTheStoredOne() {
  const QString first = writeSourcePdf(u"first.pdf"_s, "%PDF-1.4 first");
  const QString second = writeSourcePdf(u"second.pdf"_s, "%PDF-1.4 second");
  QVERIFY(!first.isEmpty());
  QVERIFY(!second.isEmpty());
  QVERIFY(!_store->storePdf(kIsbn, first).isEmpty());

  const QString stored = _store->storePdf(kIsbn, second);

  QVERIFY(!stored.isEmpty());
  QFile file(stored);
  QVERIFY(file.open(QIODevice::ReadOnly));
  QCOMPARE(file.readAll(), QByteArray{"%PDF-1.4 second"});
}

void BookFileStoreTest::storePdf_missingSource_returnsNothing() {
  const QString stored = _store->storePdf(kIsbn, QDir{_incoming->path()}.absoluteFilePath(u"nope.pdf"_s));

  QVERIFY(stored.isEmpty());
  QVERIFY(!QFile::exists(_store->pdfPath(kIsbn)));
}

void BookFileStoreTest::storePdf_withoutAnIsbn_returnsNothing() {
  const QString source = writeSourcePdf(u"book.pdf"_s);
  QVERIFY(!source.isEmpty());

  QVERIFY(_store->storePdf(0, source).isEmpty());
}

void BookFileStoreTest::storePdf_namesTheFileAfterTheBook() {
  // Named by isbn so cleanup on delete is a lookup rather than a search.
  QVERIFY(_store->pdfPath(kIsbn).contains(QString::number(kIsbn)));
  QVERIFY(_store->pdfPath(kIsbn).endsWith(u".pdf"_s));
  QVERIFY(_store->pdfPath(kIsbn) != _store->pdfPath(kOtherIsbn));
}

void BookFileStoreTest::storePdf_reStoringTheStoredFile_keepsIt() {
  // "Replace PDF" opens a file dialog that can navigate into the store itself,
  // so the source may already be the target. Copying a file over itself must not
  // destroy it.
  const QString source = writeSourcePdf(u"book.pdf"_s, "%PDF-1.4 original");
  QVERIFY(!source.isEmpty());
  const QString stored = _store->storePdf(kIsbn, source);
  QVERIFY(!stored.isEmpty());

  QCOMPARE(_store->storePdf(kIsbn, stored), stored);

  QVERIFY(QFile::exists(stored));
  QFile file(stored);
  QVERIFY(file.open(QIODevice::ReadOnly));
  QCOMPARE(file.readAll(), QByteArray{"%PDF-1.4 original"});
}

void BookFileStoreTest::storeCover_writesAPng() {
  QImage cover(20, 30, QImage::Format_RGB32);
  cover.fill(Qt::blue);

  const QString stored = _store->storeCover(kIsbn, cover);

  QVERIFY(!stored.isEmpty());
  QCOMPARE(stored, _store->coverPath(kIsbn));
  const QImage read(stored);
  QVERIFY(!read.isNull());
  QCOMPARE(read.size(), cover.size());
}

void BookFileStoreTest::storeCover_nullImage_returnsNothing() {
  QVERIFY(_store->storeCover(kIsbn, QImage{}).isEmpty());
  QVERIFY(!QFile::exists(_store->coverPath(kIsbn)));
}

void BookFileStoreTest::coverUrl_pointsAtTheStoredFile() {
  QImage cover(20, 30, QImage::Format_RGB32);
  cover.fill(Qt::blue);
  QVERIFY(!_store->storeCover(kIsbn, cover).isEmpty());

  const QString url = _store->coverUrl(kIsbn);

  QVERIFY(url.startsWith(u"file://"_s));
  // The version rides in the query, so the url still resolves to the file.
  QCOMPARE(QUrl{url}.toLocalFile(), _store->coverPath(kIsbn));
}

void BookFileStoreTest::coverUrl_changesWhenTheCoverDoes() {
  // Replacing a pdf re-renders to the same file name; Image caches by url, so an
  // unchanged url would keep showing the previous book's first page.
  QImage first(20, 30, QImage::Format_RGB32);
  first.fill(Qt::blue);
  QVERIFY(!_store->storeCover(kIsbn, first).isEmpty());
  const QString before = _store->coverUrl(kIsbn);

  QImage second(20, 30, QImage::Format_RGB32);
  second.fill(Qt::red);
  QVERIFY(!_store->storeCover(kIsbn, second).isEmpty());

  QVERIFY(!before.isEmpty());
  QVERIFY(_store->coverUrl(kIsbn) != before);
}

void BookFileStoreTest::coverUrl_sameCover_staysTheSame() {
  // Versioned by content, not by time: re-storing the same picture must not
  // invalidate a perfectly good cached image.
  QImage cover(20, 30, QImage::Format_RGB32);
  cover.fill(Qt::blue);
  QVERIFY(!_store->storeCover(kIsbn, cover).isEmpty());
  const QString before = _store->coverUrl(kIsbn);

  QVERIFY(!_store->storeCover(kIsbn, cover).isEmpty());

  QCOMPARE(_store->coverUrl(kIsbn), before);
}

void BookFileStoreTest::coverUrl_withoutAFile_isEmpty() { QVERIFY(_store->coverUrl(kIsbn).isEmpty()); }

void BookFileStoreTest::removePdf_deletesIt() {
  const QString source = writeSourcePdf(u"book.pdf"_s);
  QVERIFY(!source.isEmpty());
  QVERIFY(!_store->storePdf(kIsbn, source).isEmpty());

  QVERIFY(_store->removePdf(kIsbn));

  QVERIFY(!QFile::exists(_store->pdfPath(kIsbn)));
}

void BookFileStoreTest::removePdf_whenThereIsNone_stillReportsSuccess() {
  // The end state the caller asked for already holds.
  QVERIFY(_store->removePdf(kIsbn));
}

void BookFileStoreTest::removePdf_leavesTheCoverBehind() {
  // Once rendered, the cover is the book's picture — dropping the pdf does not
  // take it away.
  QImage cover(10, 10, QImage::Format_RGB32);
  cover.fill(Qt::red);
  QVERIFY(!_store->storeCover(kIsbn, cover).isEmpty());
  const QString source = writeSourcePdf(u"book.pdf"_s);
  QVERIFY(!_store->storePdf(kIsbn, source).isEmpty());

  QVERIFY(_store->removePdf(kIsbn));

  QVERIFY(QFile::exists(_store->coverPath(kIsbn)));
}

void BookFileStoreTest::removeAll_dropsBothFiles() {
  QImage cover(10, 10, QImage::Format_RGB32);
  cover.fill(Qt::red);
  QVERIFY(!_store->storeCover(kIsbn, cover).isEmpty());
  const QString source = writeSourcePdf(u"book.pdf"_s);
  QVERIFY(!_store->storePdf(kIsbn, source).isEmpty());

  _store->removeAll(kIsbn);

  QVERIFY(!QFile::exists(_store->pdfPath(kIsbn)));
  QVERIFY(!QFile::exists(_store->coverPath(kIsbn)));
}

void BookFileStoreTest::removeAll_leavesOtherBooksAlone() {
  const QString source = writeSourcePdf(u"book.pdf"_s);
  QVERIFY(!source.isEmpty());
  QVERIFY(!_store->storePdf(kIsbn, source).isEmpty());
  QVERIFY(!_store->storePdf(kOtherIsbn, source).isEmpty());

  _store->removeAll(kIsbn);

  QVERIFY(QFile::exists(_store->pdfPath(kOtherIsbn)));
}

void BookFileStoreTest::toLocalPath_convertsAFileUrl() {
  const QString path = QDir{_incoming->path()}.absoluteFilePath(u"book.pdf"_s);
  const QString url = QUrl::fromLocalFile(path).toString();
  QVERIFY(url.startsWith(u"file://"_s));

  QCOMPARE(BookFileStore::toLocalPath(url), path);
}

void BookFileStoreTest::toLocalPath_passesANonLocalUrlThrough() {
  // Android hands over `content://…`, which has no local path but which Qt's
  // file engine opens directly.
  const QString uri = u"content://com.android.providers.downloads/document/42"_s;

  QCOMPARE(BookFileStore::toLocalPath(uri), uri);
}

QTEST_GUILESS_MAIN(BookFileStoreTest)
#include "BookFileStoreTest.moc"
