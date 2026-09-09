# Tests

Good early tests to add:

1. `read16` / `read32` little-endian behavior
2. EWRAM and IWRAM mirroring
3. ROM mapping at `0x08000000`
4. ARM condition-code evaluation
5. Barrel shifter behavior
6. ADD/SUB flag calculations
7. Branch offset sign extension
8. THUMB decoder tests

A lightweight framework such as Catch2 or GoogleTest can be added later.
