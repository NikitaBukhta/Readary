#ifndef READARY_MODELS_BOOKS_FILTERS_BOOKFILTERSTRATEGY_HPP
#define READARY_MODELS_BOOKS_FILTERS_BOOKFILTERSTRATEGY_HPP

namespace readary::models {

class BookSortFilterProxyModel;

namespace filters {

class BookFilterStrategy {
public:
  virtual ~BookFilterStrategy() = default;
  virtual void apply(BookSortFilterProxyModel *proxy) const = 0;
};

class WantToReadFilterStrategy final : public BookFilterStrategy {
public:
  void apply(BookSortFilterProxyModel *proxy) const override;
};

class WantToBuyFilterStrategy final : public BookFilterStrategy {
public:
  void apply(BookSortFilterProxyModel *proxy) const override;
};

class AlreadyReadFilterStrategy final : public BookFilterStrategy {
public:
  void apply(BookSortFilterProxyModel *proxy) const override;
};

class ReadInProgressFilterStrategy final : public BookFilterStrategy {
public:
  void apply(BookSortFilterProxyModel *proxy) const override;
};

} // namespace filters
} // namespace readary::models

#endif // READARY_MODELS_BOOKS_FILTERS_BOOKFILTERSTRATEGY_HPP
