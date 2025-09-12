# AGENTS

This repository houses the cxb C++ utility library and its tests.

## Layout
- `cxb/` – core library headers and sources
- `tests/` – unit tests

## PR instructions
- Title format: [codex] <Title>
- Always format the code before committing with `./scripts/format.sh`

## Development
Use `rg` for searching the codebase.

### Environment Setup
Preferred: use Nix to match CI:
```bash
nix develop ./ci --system aarch64-linux --command bash ./ci/ci.sh
```
If Nix installation fails (e.g., HTTP 403) or is unavailable, install clang and build manually:
```bash
apt-get update && apt-get install -y clang
CXX=clang++ CC=clang cmake -S . -B build -DCXB_BUILD_TESTS=ON
CXX=clang++ CC=clang cmake --build build
ctest --test-dir build
```

### Building and Testing
Before building tests, fetch submodules:
`git submodule update --init --recursive`

Configure and build tests:
```bash
cmake -S . -B build -DCXB_BUILD_TESTS=ON
cmake --build build
```

Run tests:
```bash
ctest --test-dir build
```

### Manual Building and Testing
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCXB_BUILD_EXAMPLES=ON -DCXB_BUILD_TESTS=ON
cmake --build build
cd build && ctest --output-on-failure
# OR run test executable directly
./build/test_arena
```

### Code Formatting
This project uses clang-format for consistent code formatting. See `.clang-format`.

All pull requests must pass formatting checks and tests before merging.

Format all files:
```bash
./scripts/format.sh
```

Check formatting without making changes:
```bash
./scripts/format.sh --check
```

Format specific files manually:
```bash
clang-format -i path/to/file.cpp
```

### CI/CD
NixOS is used for the CI, which enables debugging the CI easier. See `.github/workflows/ci.yml` and `scripts/ci/`.
Steps performed on CI:
- Format code
- Run tests across x86_64, aarch64 on MacOS and Linux (Ubuntu)

## Style Guide
- Provide C compatible code where possible.
- Avoid the C++ standard library when cxb provides equivalent functionality (e.g., use `String8` instead of `std::string`).
- Do not add redundant comments.

### Functions
- Mark pure functions with `CXB_PURE`.
- Mark functions you truly want inlined with `CXB_INLINE`. This will force an inline when no debug information should be generated.
- Prefer free form functions over member functions.
- When performing an operation (verb) on a type instance (noun), free form functions should have the naming convention `<noun>_<verb>`.
- Exceptions include:
  - common operations performed across many different types, e.g. `make_<type>`
  - overloaded functions such as `serialize(TypeA&)` and `serialize(TypeB&)`
- If the function operates on a C-compatible POD type, do not use overloading.

### Control Flow
- Use `LIKELY()`/`UNLIKELY()` for branch prediction hints.

### Assertions and Errors
- Assert conditions with `DEBUG_ASSERT()` for debug-info compatible builds and `ASSERT()` for all builds.
- In C++, do not use exceptions.
- Use `Result<T, ErrorType>` to return errors.
  - `Result<T, ErrorType>` contains an error string; allocate the error string on the permanent arena, or pass one in if the error can be recovered.
  - Success in the `ErrorType` must be represented by the integer `0`.
- Use `Optional<T>` for values that may not exist, but not for errors.
- For C compatible APIs, you can declare a struct:
```cpp
DECALRE_RESULT_TYPE(ResultFoo, Foo, ErrorCode);

// equivalent to:
struct ResultFoo {
    Foo f;
    ErrorCode error;
    String8 reason;

    #ifdef __cplusplus
    inline operator bool() const {
        return (i64) error != 0;
    }
    #endif // __cplusplus
};
```
- For Optional:
```cpp
DECLARE_OPTIONAL_TYPE(OptionalFoo, Foo);

// equivalent to:
struct OptionalFoo {
    Foo value;
    bool exists;

    #ifdef __cplusplus
    inline operator bool() const {
        return exists;
    }
    #endif // __cplusplus
};
```

### Primitive Types
- Prefer typedefs with explicit bit size over builtin primitives, such as `u8`, `i32`, `i64`, etc.

### Composite Types
- Design around Zero Is Initialization (ZII), if possible.
- Prefer POD types (no ctors or dtors), if possible.
- Do not use `private` or `protected`, ever.
- Implement member functions as free form functions when possible. Exceptions include ctors/dtors, assignment/move operators, etc.

### Memory Management & Containers
Memory is managed with either an arena or the general heap.
- Prefer allocating memory with an `Arena`.
  - access `get_perm()` to get the permanent arena.
  - call `begin_scratch()` and `end_scratch()` for a temporary arena; assign to `AArenaTmp` for a scoped `end_scratch()`.
- Container naming convention:
  - `M` prefix indicates that manual memory management is mandatory, use `.destroy()` to free memory.
  - `A` prefix indicates that the container will automatically call `.destroy()` using RAII.

### Templates and Generics
- Minimize template complexity.
- Don't use templates unless it is necessary.
- Use SFINAE sparingly and only when necessary: `std::enable_if_t<std::is_integral_v<T>>`.
- Prefer explicit instantiation over heavy template metaprogramming.

### Parallel Code
- For C compatible APIs: use `atomic_i64`, `atomic_u64`, etc. typedefs or C11's `_Atomic(T)`.
- In C++: use the `Atomic<T>` wrapper.
- Use explicit memory ordering: C11 `memory_order` constants directly.

### Names and Keywords
#### Macros
- All uppercase with underscores: `CXB_INLINE`, `ASSERT`, `BREAKPOINT`.
- Library-specific macros prefixed with `CXB_`: `CXB_EXPORT`, `CXB_INTERNAL`.

#### Types
- Prefer the `struct` keyword over `class`.
- PascalCase for classes and structs: `Allocator`, `Utf8Iterator`.
- Lowercase abbreviations for primitive type aliases: `f32`, `f64`, `i32`, `u64`.
- Descriptive compound names: `Vec2f`, `Color4i`, `Utf8DecodeResult`.

#### Functions and Variables
- `snake_case` for functions.
- `snake_case` for member variables: `n_active_bytes`, `bytes_consumed`.

#### Constants and Enums
- C enum values should be UPPER_CASE.
- C++ enum class values use PascalCase (e.g. `Relaxed`, `Acquire`, `SeqCst`).
- `snake_case` for static constants: `identity4x4`, `identity3x3`.

