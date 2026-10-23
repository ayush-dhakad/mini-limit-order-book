#include "order_book.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
void print_trades(const std::vector<lob::Trade>& trades) {
    for (const auto& trade : trades)
        std::cout << "TRADE maker=" << trade.maker_id << " taker=" << trade.taker_id
                  << " price=" << trade.price << " qty=" << trade.quantity << '\n';
}

void demo() {
    lob::OrderBook book;
    for (const lob::Order order : {
             lob::Order{1, lob::Side::Sell, 101, 5}, {2, lob::Side::Sell, 101, 4},
             {3, lob::Side::Sell, 102, 3}, {4, lob::Side::Buy, 99, 6}}) {
        print_trades(book.add(order));
    }
    book.display(std::cout);
    std::cout << "\nBUY 5 102 7: FIFO fills order 1, then part of order 2\n";
    print_trades(book.add({5, lob::Side::Buy, 102, 7}));
    book.display(std::cout);
    std::cout << "\nCANCEL 2: " << (book.cancel(2) ? "cancelled" : "not found") << '\n';
    book.display(std::cout);
}

void interactive() {
    lob::OrderBook book;
    std::cout << "Commands: BUY id price qty | SELL id price qty | CANCEL id | BOOK | QUIT\n";
    std::string line;
    while (std::cout << "> " && std::getline(std::cin, line)) {
        std::istringstream input(line);
        std::string command, extra;
        if (!(input >> command)) continue;
        try {
            if (command == "BUY" || command == "SELL") {
                // Parse as signed so a negative ID cannot wrap to an unsigned ID.
                std::int64_t id, price, quantity;
                if (!(input >> id >> price >> quantity) || (input >> extra) || id <= 0)
                    throw std::invalid_argument("expected: BUY/SELL positive-id price quantity");
                print_trades(book.add({static_cast<lob::OrderId>(id),
                    command == "BUY" ? lob::Side::Buy : lob::Side::Sell, price, quantity}));
            } else if (command == "CANCEL") {
                std::int64_t id;
                if (!(input >> id) || (input >> extra) || id <= 0)
                    throw std::invalid_argument("expected: CANCEL positive-id");
                std::cout << (book.cancel(static_cast<lob::OrderId>(id)) ? "Cancelled\n" : "ID not found\n");
            } else if (command == "BOOK" || command == "QUIT") {
                if (input >> extra) throw std::invalid_argument("unexpected extra argument");
                if (command == "QUIT") return;
                book.display(std::cout);
            } else {
                throw std::invalid_argument("unknown command (commands are uppercase)");
            }
        } catch (const std::invalid_argument& error) {
            std::cout << "Error: " << error.what() << '\n';
        }
    }
}
} // namespace

int main(int argc, char* argv[]) {
    if (argc == 1) demo();
    else if (argc == 2 && std::string(argv[1]) == "--interactive") interactive();
    else {
        std::cerr << "Usage: lob_demo [--interactive]\n";
        return 1;
    }
}
