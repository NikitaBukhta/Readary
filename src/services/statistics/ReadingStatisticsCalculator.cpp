#include "services/statistics/ReadingStatisticsCalculator.hpp"

#include "services/dto/BookStatus.hpp"
#include "services/statistics/BookStatisticsCalculator.hpp"

#include <QDateTime>
#include <QHash>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <utility>
#include <vector>

namespace {

using readary::services::BookDTO;
using readary::services::BookProgressDTO;
using readary::services::BookStatus;
using readary::services::PeriodBucketDTO;
using readary::services::ReadingSessionDTO;
using readary::services::ReadingStatisticsDTO;
using readary::services::StatisticsGranularity;
using readary::services::StatisticsRange;

std::vector<PeriodBucketDTO> emptyBuckets(const StatisticsRange &range) {
  const int count = std::max(0, range.bucketCount());
  std::vector<PeriodBucketDTO> buckets;
  buckets.reserve(static_cast<std::size_t>(count));
  for (int i = 0; i < count; ++i) {
    buckets.push_back({.date = range.bucketStart(i), .hour = range.granularity == StatisticsGranularity::Hour ? i : 0});
  }
  return buckets;
}

PeriodBucketDTO *bucketAt(std::vector<PeriodBucketDTO> &buckets, int index) {
  if (index < 0 || static_cast<std::size_t>(index) >= buckets.size()) {
    return nullptr;
  }
  return &buckets.at(static_cast<std::size_t>(index));
}

QHash<qint64, ReadingSessionDTO> lastSessionByBook(const QList<ReadingSessionDTO> &sessions) {
  QHash<qint64, ReadingSessionDTO> last;
  for (const ReadingSessionDTO &session : sessions) {
    const auto known = last.constFind(session.bookIsbn);
    if (known == last.cend() || known->startedBefore(session)) {
      last.insert(session.bookIsbn, session);
    }
  }
  return last;
}

int countFinished(const QList<BookDTO> &books, const QHash<qint64, ReadingSessionDTO> &lastSession,
                  const StatisticsRange &range, std::vector<PeriodBucketDTO> &buckets) {
  int finished = 0;
  for (const BookDTO &book : books) {
    if (book.status != BookStatus::Finished) {
      continue;
    }
    const auto last = lastSession.constFind(book.isbn);
    if (last == lastSession.cend()) {
      if (range.unbounded) {
        ++finished;
      }
      continue;
    }
    const QDateTime &finishedAt = last->startedAt;
    if (range.unbounded || range.contains(finishedAt.date())) {
      ++finished;
    }
    if (PeriodBucketDTO *bucket = bucketAt(buckets, range.bucketOf(finishedAt))) {
      ++bucket->books;
    }
  }
  return finished;
}

struct Activity {
  ReadingSessionDTO first;
  ReadingSessionDTO last;
  int pages = 0;
};

QHash<qint64, Activity> activityByBook(const QList<ReadingSessionDTO> &inRange) {
  QHash<qint64, Activity> activity;
  for (const ReadingSessionDTO &session : inRange) {
    const auto known = activity.find(session.bookIsbn);
    if (known == activity.end()) {
      activity.insert(session.bookIsbn, {.first = session, .last = session, .pages = session.pagesRead()});
      continue;
    }
    if (session.startedBefore(known->first)) {
      known->first = session;
    }
    if (known->last.startedBefore(session)) {
      known->last = session;
    }
    known->pages += session.pagesRead();
  }
  return activity;
}

struct ProgressRow {
  BookProgressDTO progress;
  QDateTime lastRead;
};

QList<BookProgressDTO> booksReadIn(const QList<BookDTO> &books, const QHash<qint64, Activity> &activity,
                                   bool unbounded) {
  QList<ProgressRow> rows;
  for (const BookDTO &book : books) {
    const auto read = activity.constFind(book.isbn);
    if (read != activity.cend()) {
      rows.append({.progress = {.isbn = book.isbn,
                                .name = book.name,
                                .fromPage = read->first.pagesFrom,
                                .toPage = read->last.endPage(),
                                .pagesInPeriod = read->pages,
                                .totalPages = std::max(0, book.totalPages)},
                   .lastRead = read->last.startedAt});
    } else if (unbounded && book.status == BookStatus::InProgress) {
      const int position = std::max(0, book.pagesRead);
      rows.append({.progress = {.isbn = book.isbn,
                                .name = book.name,
                                .fromPage = position,
                                .toPage = position,
                                .totalPages = std::max(0, book.totalPages)},
                   .lastRead = {}});
    }
  }

  std::ranges::sort(rows, [](const ProgressRow &lhs, const ProgressRow &rhs) {
    if (lhs.lastRead != rhs.lastRead) {
      return lhs.lastRead.isValid() && (!rhs.lastRead.isValid() || lhs.lastRead > rhs.lastRead);
    }
    const int byName = lhs.progress.name.compare(rhs.progress.name, Qt::CaseInsensitive);
    return byName != 0 ? byName < 0 : lhs.progress.isbn < rhs.progress.isbn;
  });

  QList<BookProgressDTO> shown;
  shown.reserve(std::min(rows.size(), ReadingStatisticsDTO::kBooksShown));
  for (ProgressRow &row : rows) {
    if (shown.size() == ReadingStatisticsDTO::kBooksShown) {
      break;
    }
    shown.append(std::move(row.progress));
  }
  return shown;
}

} // namespace

namespace readary::services {

QDate ReadingStatisticsCalculator::earliestSession(const QList<ReadingSessionDTO> &sessions) {
  QDate earliest;
  for (const ReadingSessionDTO &session : sessions) {
    const QDate started = session.startedAt.date();
    if (started.isValid() && (!earliest.isValid() || started < earliest)) {
      earliest = started;
    }
  }
  return earliest;
}

ReadingStatisticsDTO ReadingStatisticsCalculator::compute(const QList<BookDTO> &books,
                                                          const QList<ReadingSessionDTO> &sessions,
                                                          const StatisticsRange &range) {
  QList<ReadingSessionDTO> inRange;
  for (const ReadingSessionDTO &session : sessions) {
    if (range.unbounded || range.contains(session.startedAt.date())) {
      inRange.append(session);
    }
  }

  ReadingStatisticsDTO stats{BookStatisticsCalculator::journalFigures(inRange)};
  stats.range = range;

  std::vector<PeriodBucketDTO> buckets = emptyBuckets(range);
  for (const ReadingSessionDTO &session : inRange) {
    if (PeriodBucketDTO *bucket = bucketAt(buckets, range.bucketOf(session.startedAt))) {
      bucket->pages += session.pagesRead();
    }
  }
  stats.booksFinished = countFinished(books, lastSessionByBook(sessions), range, buckets);
  stats.buckets =
      QList<PeriodBucketDTO>(std::make_move_iterator(buckets.begin()), std::make_move_iterator(buckets.end()));

  stats.booksRead = booksReadIn(books, activityByBook(inRange), range.unbounded);
  return stats;
}

} // namespace readary::services
