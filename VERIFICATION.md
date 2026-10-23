# Verification

Verified on macOS with Apple Clang using C++17:

- Warning-clean builds with `-Wall -Wextra -Wpedantic -Werror`.
- All 34 checks across six test scenarios passed with AddressSanitizer and
  UndefinedBehaviorSanitizer enabled.
- All 34 checks passed in an optimized `-O2 -DNDEBUG` build.
- The scripted demo ran successfully and produced the documented FIFO trades.
- The interactive CLI was exercised with matching, book display, cancellation,
  missing IDs, negative IDs, extra arguments and numeric overflow.

CMake was unavailable in the verification environment, so compilation used
the direct compiler commands documented in README.md. The supplied CMake/CTest
configuration has not been executed in this environment.
