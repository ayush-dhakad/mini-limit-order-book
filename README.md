# Mini Limit Order Book — C++17

A small, single-instrument matching engine for a placement portfolio. It supports
buy/sell limit orders, best-price then arrival-time matching, partial fills,
cancellation by ID, and terminal book/trade output. Standard library only.

## Build and run

Requirements: a C++17 compiler and CMake 3.16+. From this directory:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
./build/lob_demo
./build/lob_demo --interactive
```

For Windows multi-configuration generators, run `build\Debug\lob_demo.exe`.
The default demo seeds the book, buys seven units against two FIFO sell orders,
cancels the remaining second sell order, and prints each book state. Its trades:

```text
TRADE maker=1 taker=5 price=101 qty=5
TRADE maker=2 taker=5 price=101 qty=2
```

Interactive example (commands are uppercase):

```text
SELL 1 101 5
SELL 2 101 4
BUY 3 102 7
BOOK
CANCEL 2
BOOK
QUIT
```

`BUY` and `SELL` take `id price quantity`. Invalid commands, extra arguments,
numeric overflow, nonpositive fields and duplicate active IDs are rejected.
EOF exits. The CLI accepts IDs through signed 64-bit maximum; the library uses
unsigned 64-bit IDs. No CMake available? Build directly:

```sh
c++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude src/order_book.cpp src/main.cpp -o lob_demo
c++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude src/order_book.cpp tests/order_book_tests.cpp -o lob_tests
./lob_tests
./lob_demo
```

## Matching rules

- Prices are positive integer **ticks**, and quantities are positive integer units.
  Choose a tick value for your hypothetical instrument; there is no floating-point money.
- Highest bid and lowest ask have priority. Equal-price orders execute FIFO.
- An incoming buy crosses asks at or below its limit; an incoming sell crosses
  bids at or above its limit. Trades execute at the resting maker's price.
- A partial maker keeps its FIFO position. Any unfilled incoming quantity rests
  at its original limit, at the back of that price level.
- Only active resting IDs must be unique. Filled/cancelled IDs may be reused,
  and reuse receives a new FIFO position. Cancelling an unknown ID returns false.
- No amend/replace, market orders, expiry, fees, self-trade prevention or persistent
  trade history. `add()` returns that submission's trades; the caller owns them.

## Structure and design

```text
include/order_book.hpp        Public types and matching-engine API
src/order_book.cpp            Matching, cancellation, snapshots and printing
src/main.cpp                  Scripted demo and optional interactive CLI
tests/order_book_tests.cpp    Standalone test executable; no framework
CMakeLists.txt               Library, demo, tests and CTest registration
```

Bids use `std::map<Price, std::list<Order>, std::greater<Price>>`; asks use an
ascending map. Each list stores one price level in arrival order. An
`std::unordered_map<OrderId, Location>` points to list iterators for direct
order lookup. List iterators remain valid when other orders are added or erased.
An empty price level is removed immediately. Copy/move are disabled because
the index holds iterators into the owning book.

Core invariants: every resting order has positive remaining quantity and exactly
one index entry; no price level is empty; the resting book is never crossed after
a submission. Matching only subtracts the smaller quantity, so it avoids sums
or price-times-quantity calculations that could overflow.

## Complexity

Let **N** be resting orders, **L** active price levels, and **F** maker orders
touched by an incoming order (each produces one trade).

| Operation | Expected time | Notes |
|---|---|---|
| Add without a match | O(log L) | Find/create level, append FIFO, index ID |
| Add with matches | O(F + log L) | Begin/front access and iterator erases are amortized constant; optional remainder insertion |
| Cancel by ID | O(log L) | Expected O(1) hash lookup, O(log L) level lookup, O(1) list erase |
| Snapshot / print | O(N) | Traverses resting orders; snapshot allocates a copy |
| Resting storage | O(N + L) | Orders, levels and ID index; returned trades add O(F) |

Hash operations are expected/amortized O(1), with O(N) worst-case lookup/erase
under collisions. Rehashing may make an individual insertion O(N); matching's
hash erases can degrade to O(FN) in a collision-heavy worst case. These are
standard-library container bounds, not latency guarantees.

## Tests and scope

Tests cover an empty book, a non-crossing spread, FIFO, both aggressor sides,
maker-price execution, multi-level sweeps, partial makers, resting taker remainders,
limit boundaries, cancellation at the head/middle/tail, level cleanup, ID reuse,
invalid fields, duplicate rejection before matching and large integer values.
Checks remain enabled in Release builds (they do not use `assert`).

This is an educational engine for one instrument in one process. No database,
networking, external data, threading or trading strategy is included. Arrival
order means the order in which the caller invokes `add()`. Allocation failures
are not transactionally rolled back; this is not a production exchange engine.

For an interview, trace `match()` first, then explain why lists allow FIFO plus
constant-time removal and why cancellation still includes a map lookup.
# mini-limit-order-book
# mini-limit-order-book
