#include "order_book.hpp"

#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
int checks = 0;
void check(bool condition, const char* description) {
    ++checks;
    if (!condition) throw std::runtime_error(description);
}
template <typename Function>
void rejects(Function function) {
    bool rejected = false;
    try { function(); } catch (const std::invalid_argument&) { rejected = true; }
    check(rejected, "invalid input must throw invalid_argument");
}

void empty_and_non_crossing() {
    lob::OrderBook book;
    check(book.size() == 0 && !book.cancel(99), "empty book");
    check(book.add({1, lob::Side::Buy, 99, 4}).empty(), "rest bid");
    check(book.add({2, lob::Side::Sell, 101, 7}).empty(), "spread must not cross");
    check(book.size() == 2, "two resting orders");
    std::ostringstream output;
    book.display(output);
    check(output.str().find("id=1 price=99 qty=4") != std::string::npos, "display bid");
}

void fifo_and_partial_fill() {
    lob::OrderBook book;
    book.add({1, lob::Side::Sell, 100, 5});
    book.add({2, lob::Side::Sell, 100, 5});
    const auto trades = book.add({3, lob::Side::Buy, 101, 7});
    check(trades.size() == 2, "two FIFO fills");
    check(trades[0].maker_id == 1 && trades[0].taker_id == 3 &&
          trades[0].price == 100 && trades[0].quantity == 5, "first maker and maker price");
    check(trades[1].maker_id == 2 && trades[1].quantity == 2, "second maker partially filled");
    const auto asks = book.orders(lob::Side::Sell);
    check(asks.size() == 1 && asks[0].id == 2 && asks[0].quantity == 3, "partial remainder");
    check(!book.cancel(1) && !book.cancel(3) && book.cancel(2), "only resting orders cancel");
    check(book.size() == 0, "all orders removed");
}

void price_priority_and_remainder() {
    lob::OrderBook book;
    book.add({1, lob::Side::Sell, 103, 2});
    book.add({2, lob::Side::Sell, 101, 3});
    book.add({3, lob::Side::Sell, 102, 4});
    const auto trades = book.add({4, lob::Side::Buy, 102, 10});
    check(trades.size() == 2 && trades[0].maker_id == 2 && trades[1].maker_id == 3,
          "best ask before older worse asks");
    const auto bids = book.orders(lob::Side::Buy);
    check(bids.size() == 1 && bids[0].id == 4 && bids[0].quantity == 3 &&
          bids[0].price == 102, "unfilled taker rests at limit");
    check(book.orders(lob::Side::Sell)[0].id == 1, "limit prevents further matching");
}

void sell_aggressor() {
    lob::OrderBook book;
    book.add({1, lob::Side::Buy, 100, 2});
    book.add({2, lob::Side::Buy, 102, 3});
    book.add({3, lob::Side::Buy, 102, 4});
    auto trades = book.add({4, lob::Side::Sell, 100, 8});
    check(trades.size() == 3 && trades[0].maker_id == 2 && trades[1].maker_id == 3 &&
          trades[2].maker_id == 1, "descending bid priority and FIFO");
    check(trades[0].price == 102 && trades[2].price == 100, "bid maker prices");
    check(book.orders(lob::Side::Buy)[0].quantity == 1, "partial bid remains");
    trades = book.add({5, lob::Side::Sell, 100, 1});
    check(trades.size() == 1 && book.size() == 0, "exact fill cleans final level");
}

void cancellation_and_id_reuse() {
    lob::OrderBook book;
    for (lob::OrderId id = 1; id <= 4; ++id) book.add({id, lob::Side::Buy, 100, 1});
    check(book.cancel(2), "cancel middle");
    check(book.cancel(1), "cancel head");
    check(book.cancel(4), "cancel tail");
    check(!book.cancel(4), "repeat cancellation");
    book.add({2, lob::Side::Buy, 100, 1});
    auto trades = book.add({5, lob::Side::Sell, 100, 2});
    check(trades.size() == 2 && trades[0].maker_id == 3 && trades[1].maker_id == 2,
          "reused cancelled ID joins back of FIFO");
    book.add({5, lob::Side::Sell, 105, 1});
    check(book.cancel(5) && book.size() == 0, "filled taker ID reusable; cancel last ask");
}

void validation_and_boundaries() {
    lob::OrderBook book;
    rejects([&] { book.add({0, lob::Side::Buy, 1, 1}); });
    rejects([&] { book.add({1, lob::Side::Buy, 0, 1}); });
    rejects([&] { book.add({1, lob::Side::Buy, -1, 1}); });
    rejects([&] { book.add({1, lob::Side::Buy, 1, 0}); });
    rejects([&] { book.add({1, lob::Side::Buy, 1, -1}); });
    rejects([&] { book.add({1, static_cast<lob::Side>(9), 1, 1}); });
    book.add({1, lob::Side::Sell, 100, 2});
    rejects([&] { book.add({1, lob::Side::Buy, 101, 2}); });
    check(book.size() == 1 && book.orders(lob::Side::Sell)[0].quantity == 2,
          "duplicate rejected before any matching");
    check(book.cancel(1), "clear validation order");
    const auto maximum = std::numeric_limits<std::int64_t>::max();
    book.add({1, lob::Side::Buy, maximum, maximum});
    auto trades = book.add({2, lob::Side::Sell, maximum, maximum});
    check(trades.size() == 1 && trades[0].quantity == maximum && book.size() == 0,
          "large values do not require price multiplication or quantity aggregation");
}
} // namespace

int main() {
    try {
        empty_and_non_crossing();
        fifo_and_partial_fill();
        price_priority_and_remainder();
        sell_aggressor();
        cancellation_and_id_reuse();
        validation_and_boundaries();
        std::cout << "PASS: " << checks << " checks across 6 test scenarios\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL after " << checks << " checks: " << error.what() << '\n';
        return 1;
    }
}
