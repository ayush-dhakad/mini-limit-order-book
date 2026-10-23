#pragma once

#include <cstdint>
#include <functional>
#include <iosfwd>
#include <list>
#include <map>
#include <unordered_map>
#include <vector>

namespace lob {
using OrderId = std::uint64_t;
using Price = std::int64_t; // Integer ticks; never floating point.
using Quantity = std::int64_t;
enum class Side { Buy, Sell };

struct Order {
    OrderId id;
    Side side;
    Price price;
    Quantity quantity; // Remaining quantity for resting orders.
};

struct Trade {
    OrderId maker_id;
    OrderId taker_id;
    Price price;
    Quantity quantity;
};

class OrderBook {
public:
    OrderBook() = default;
    // The ID index contains iterators into this object's lists.
    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;
    OrderBook(OrderBook&&) = delete;
    OrderBook& operator=(OrderBook&&) = delete;

    // Throws invalid_argument for invalid fields or an already-active ID.
    // Returned trades are in execution order, at the resting maker's price.
    std::vector<Trade> add(Order order);
    bool cancel(OrderId id);
    std::vector<Order> orders(Side side) const; // Best price first, then FIFO.
    std::size_t size() const noexcept { return index_.size(); }
    void display(std::ostream& out) const;

private:
    using Queue = std::list<Order>;
    using Bids = std::map<Price, Queue, std::greater<Price>>;
    using Asks = std::map<Price, Queue>;
    struct Location {
        Side side;
        Price price;
        Queue::iterator order;
    };
    Bids bids_;
    Asks asks_;
    std::unordered_map<OrderId, Location> index_;

    template <typename Levels>
    void match(Order& incoming, Levels& opposite, std::vector<Trade>& trades);
    template <typename Levels>
    void rest(const Order& order, Levels& levels);
};
} // namespace lob
