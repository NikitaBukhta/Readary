#ifndef READARY_TESTS_SUPPORT_SEARCHCAPTURE_HPP
#define READARY_TESTS_SUPPORT_SEARCHCAPTURE_HPP

#include "api/bookSearch/IBookSearchAPI.hpp"
#include "services/dto/BookDTO.hpp"

#include <QList>
#include <QObject>

namespace readary::tests {

struct SearchResult {
  QList<services::BookDTO> books;
  bool hasMore{true};
  bool received{false};
  int count{0};
};

inline void observeSearch(api::IBookSearchAPI &source, SearchResult &out) {
  QObject::connect(&source, &api::IBookSearchAPI::searchListUpdated, &source,
                   [&out](const QList<services::BookDTO> &books, bool hasMore) {
                     out.books = books;
                     out.hasMore = hasMore;
                     out.received = true;
                     ++out.count;
                   });
}

inline void runSearch(api::IBookSearchAPI &source, const api::BookSearchFields &fields, SearchResult &out) {
  observeSearch(source, out);
  source.search(fields);
}

} // namespace readary::tests

#endif // READARY_TESTS_SUPPORT_SEARCHCAPTURE_HPP
