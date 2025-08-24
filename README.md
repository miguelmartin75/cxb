> [!WARNING]
> Under Construction

# cxb

A C++ utility library with focus on performance and simplicity.

Current Header Compile Times:
* `cxb-cxx.h`: 131±124ms on Apple M1 Max

## Development

For development setup, building, and testing see [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md).

# TODOs
- [ ] formatting
    - [ ] Dragonbox float conversion algo
- [ ] hash map
    - [ ] HashMap<K, V> on arena with free form functions
        - hashmap_put(m, a, k, v)
        - hashmap_get(m, a, k, v)
        - hashmap_exists(m, a, k, v)
    - [ ] MHashMap<K, V>, AHashMap<K, V> on allocator
