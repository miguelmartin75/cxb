![coverage](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/miguelmartin75/cxb/refs/heads/gh-pages/coverage.json)

> [!WARNING]
> Under Construction

# cxb

An unorthodox C++ base library with a focus on performance & simplicity

Current Header Compile Times:
* `cxb-cxx.h`: 138±96ms on Apple M1 Max

## Development

For development setup, building, and testing see [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md).

## Coverage

Run `./scripts/ci/coverage.sh` to build the project with clang coverage instrumentation and generate `coverage.txt`. CI uploads this report as an artifact for the Linux clang Debug job.

# TODOs
- [ ] algorithms
    - [ ] (P0) binary search (1hr)
    - [ ] (P0) partition (1hr)
    - [ ] (P0) quick sort (1hr)
    - [ ] (P1) iterator utilities: reduce, map, filter (2hr)
    - [ ] (P1) data-structure mutation/creation: reduce, map, filter (2hr)
- [ ] data structures
    - [ ] (P1) Union-Find
    - [ ] (P1) M/AStableArray
        An array that owns an Arena, such that this can be used for the following use-case: 
        - `AHashMap<Key, AStableArray<T>>`
    - [ ] (P1) Pool<T>
        NOTE: consider merging this functionality into stable array
        Wraps an arena to pool objects
    - [ ] (P1) M/ATreap
- [ ] parallel code
    - [ ] (P0) thread pool
         - Set spin-lock mode or to wake-up with a `std::condition_variable` when used
    - [ ] (P0) par_map, par_filter, par_reduce on thread pool
- [ ] Refactor
    - [ ] (P0) C compatibility
        - [ ] (P0) Refactor free-form functions on CXB_C_TYPE's to be C compatible
            - Have current C++ variants in `cxb` namespace
        - [ ] Arena macros
    - [ ] (P0) remove C11's atomic usage in header file or fix GCC compilation
    - [ ] (P1) C++ specific code in `cxb` namespace
    - [ ] (P1) MHashMap<K, V>: don't inherit from HashMap<K, V, Hasher>
    - [ ] (P2) Group and move M/A prefixed types to bottom of header-file
    - [ ] (P2) `git mv tests/benchmarks tests/benchs` with CMakeLists.txt changes
    - [ ] (P2) `tests/fuzz/*_fuzz.cpp` -> `tests/fuzz/fuzz_*.cpp` with CMakeLists.txt changes
    - [ ] (P2) remove fmtlib dependency
        - [ ] Write the Dragonbox float conversion algorithm
- [ ] QoL
    - [ ] defer, see: https://github.com/gingerBill/gb/blob/master/gb.h#L658
- [ ] DevOps
    - [ ] Count coverage only for source files defined in `cxb/`, it is currently counting dependencies coverage (Catch2 and fmtlib's)
- [ ] Code Examples
    - [ ] `examples/interpreter.cpp`: AST-based interpreter 
        - Duck typing, all code defined in one file
        - Perf: compare to Python and Lua for simple programs
- [ ] tests, fuzzing & benchmarks
    - [ ] (P0) fuzz each algorithm
    - [ ] (P0) fuzz each data-structure
    - [ ] (P0) bench_string rename to bench_utf8.cpp and only keep UTF-8 related benchmarks
