#include "order_book.hpp"

#include <algorithm>
#include <iterator>
#include <ostream>
#include <stdexcept>

namespace lob {
template <typename Levels>
void OrderBook::match(Order& incoming, Levels& opposite, std::vector<Trade>& trades) {
    while (incoming.quantity > 0 && !opposite.empty()) {
        auto level = opposite.begin();
        const bool crosses = incoming.side == Side::Buy
            ? incoming.price >= level->first : incoming.price <= level->first;
        if (!crosses) break;

        auto& queue = level->second;
        auto& maker = queue.front();
        const auto quantity = std::min(incoming.quantity, maker.quantity);
        trades.push_back({maker.id, incoming.id, maker.price, quantity});
        incoming.quantity -= quantity;
        maker.quantity -= quantity;
        if (maker.quantity == 0) {
            index_.erase(maker.id);
            queue.pop_front();
        }
        if (queue.empty()) opposite.erase(level);
    }
}

template <typename Levels>
void OrderBook::rest(const Order& order, Levels& levels) {
    auto& queue = levels[order.price];
    queue.push_back(order);
    index_.emplace(order.id, Location{order.side, order.price, std::prev(queue.end())});
}

std::vector<Trade> OrderBook::add(Order order) {
    if (order.id == 0 || order.price <= 0 || order.quantity <= 0 ||
        (order.side != Side::Buy && order.side != Side::Sell)) {
        throw std::invalid_argument("ID, price and quantity must be positive; side must be valid");
    }
    if (index_.count(order.id)) throw std::invalid_argument("order ID is already active");

    std::vector<Trade> trades;
    if (order.side == Side::Buy) {
        match(order, asks_, trades);
        if (order.quantity > 0) rest(order, bids_);
    } else {
        match(order, bids_, trades);
        if (order.quantity > 0) rest(order, asks_);
    }
    return trades;
}

bool OrderBook::cancel(OrderId id) {
    const auto found = index_.find(id);
    if (found == index_.end()) return false;
    const auto location = found->second;
    if (location.side == Side::Buy) {
        auto level = bids_.find(location.price);
        level->second.erase(location.order);
        if (level->second.empty()) bids_.erase(level);
    } else {
        auto level = asks_.find(location.price);
        level->second.erase(location.order);
        if (level->second.empty()) asks_.erase(level);
    }
    index_.erase(found);
    return true;
}

std::vector<Order> OrderBook::orders(Side side) const {
    if (side != Side::Buy && side != Side::Sell) throw std::invalid_argument("invalid side");
    std::vector<Order> result;
    if (side == Side::Buy) {
        for (const auto& level : bids_)
            result.insert(result.end(), level.second.begin(), level.second.end());
    } else {
        for (const auto& level : asks_)
            result.insert(result.end(), level.second.begin(), level.second.end());
    }
    return result;
}

void OrderBook::display(std::ostream& out) const {
    out << "BOOK (integer ticks; quantity remaining)\n";
    for (const auto side : {Side::Sell, Side::Buy}) {
        out << (side == Side::Buy ? "BIDS" : "ASKS") << " (best first)\n";
        const auto snapshot = orders(side);
        if (snapshot.empty()) out << "  (empty)\n";
        for (const auto& order : snapshot)
            out << "  id=" << order.id << " price=" << order.price
                << " qty=" << order.quantity << '\n';
    }
}
} // namespace lob
