# Fuzzing

Build fuzz targets:

```
cmake -S . -B build -DCXB_BUILD_FUZZERS=ON
cmake --build build
```

Run a fuzz target (CI smoke test):

```
./build/hashmap_fuzz -max_total_time=60
```

