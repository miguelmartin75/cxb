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
    - [ ] remove or update utf8 decode
- [ ] std::initializer_list support
    - [ ] Array
    - [ ] MArray
    - [ ] AArray
- [ ] hash map
    - [ ] HashMap<K, V> on arena with free form functions
        - hashmap_put(m, a, k, v)
        - hashmap_get(m, a, k, v)
        - hashmap_exists(m, a, k, v)
    - [ ] MHashMap<K, V>, AHashMap<K, V> on allocator
- [ ] arena allocator
- [ ] test cases
    - [ ] arena allocator
