> [!WARNING]
> Under Construction

# cxb

An unorthodox C++ base library with a focus on performance & simplicity

Current Header Compile Times:
* `cxb-cxx.h`: 164±150ms on Apple M1 Max

## Development

For development setup, building, and testing see [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md).

# TODOs
- [ ] formatting
    - [x] print & format alternative (using fmtlib for floats)
    - [ ] Dragonbox float conversion algo
- [ ] refactor
    - [x] A/MString -> A/MString8
    - [x] remove or update utf8 decode
    - [ ] concepts: only use ArrayLikeNoT
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
