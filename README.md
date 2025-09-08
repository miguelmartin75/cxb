> [!WARNING]
> Under Construction

# cxb

An unorthodox C++ base library with a focus on performance & simplicity

Current Header Compile Times:
* `cxb-cxx.h`: 164±150ms on Apple M1 Max

![coverage](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/codex/cxb/gh-pages/coverage.json)

## Development

For development setup, building, and testing see [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md).

## Coverage

Run `./scripts/ci/coverage.sh` to build the project with clang coverage instrumentation and generate `coverage.txt`. CI uploads this report as an artifact for the Linux clang Debug job.

## Fuzzing

Build fuzz targets and run a short smoke test:

```
cmake -S . -B build -DCXB_BUILD_FUZZERS=ON
cmake --build build
./build/hashmap_fuzz -max_total_time=60
```

# TODOs
- [ ] formatting
    - [x] print & format alternative (using fmtlib for floats)
    - [ ] Dragonbox float conversion algo
- [ ] refactor
    - [x] A/MString -> A/MString8
    - [x] remove or update utf8 decode
    - [x] concepts: only use ArrayLike
    - [ ] TODO: decide on whether to make free-form functions C-compat?
- [ ] std::initializer_list support
    - [x] Array
    - [x] MArray
    - [x] AArray
- [ ] hash map
    - [ ] HashMap<K, V> on arena with free form functions
        - hashmap_put(m, a, k, v)
        - hashmap_get(m, a, k, v)
        - hashmap_exists(m, a, k, v)
    - [ ] MHashMap<K, V>, AHashMap<K, V> on allocator
- [ ] arena allocator
- [ ] test cases
    - [ ] arena allocator
    - [ ] arena: 
        - [ ] array_*
        - [ ] string8_*
    - [ ] remove or refactor bench_string
