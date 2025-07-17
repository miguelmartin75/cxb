# CXB Style Guide

## General Principles

- Prioritize clarity and consistency over brevity
- Use modern C++ features sparingly while maintaining C compatibility where possible
- Favor explicit over implicit behavior
- Keep the API surface minimal and composable

## Naming Conventions

### Macros
- All uppercase with underscores: `CXB_INLINE`, `ASSERT`, `BREAKPOINT`
- Library-specific macros prefixed with `CXB_`: `CXB_EXPORT`, `CXB_INTERNAL`
- Literal helper macros use descriptive suffixes: `S8_LIT()`, `COUNTOF_LIT()`

### Types
- Prefer the `struct` keyword over `class`
- PascalCase for classes and structs: `Allocator`, `Mallocator`, `Utf8Iterator`
- Lowercase abbreviations for primitive type aliases: `f32`, `f64`, `i32`, `u64`
- Descriptive compound names: `Vec2f`, `Color4i`, `Utf8DecodeResult`

### Functions and Variables
- `snake_case` for functions
- `snake_case` for member variables: `n_active_bytes`, `bytes_consumed`
- Do not make any variable or function "private", use a leading underscore if a variable is intended to be private, i.e. is an "implementation" detail

### Constants and Enums
- PascalCase for enum values: `Relaxed`, `Acquire`, `SeqCst`
- snake_case for static constants: `identity4x4`, `identity3x3`

## Code Organization

### Header Structure
1. Preprocessor definitions and feature detection
2. Macro definitions (general purpose first, then library-specific)
3. Type aliases for primitives
4. Core utility functions and classes
5. Data structures (simple to complex)
6. Specialized utilities and helpers

### Class/Struct Layout

Always use the struct keyword and expose all implementation details. Never use protected or private. Do not make any variable or function "private", use a leading underscore if a variable is intended to be private, i.e. is an "implementation" detail


```cpp
struct Example {
    // member variables first
    Type member_var;

    // then ctors & dtors
    Example() = default;
    ~Example() = default;

    // group assignment operators with the associated ctor
    Example(const Type& param) = default;
    Example& operator=(const Example& other);

    // now member functions
    void fn1();
    int fn2();

    // lastly include the operators
    bool operator==(const Example& other);
};
```

## Memory Management & Containers

- Prefer using the `Arena` type
- A container marked with an `M` prefix indicates that manual memory management is mandatory, use `.destroy()` to free memory
- A container marked with an `A` prefix indicates that the container will automatically call `.destroy()` using RAII mechanisms
- Use `StableArray<T>` or `AStableArray<T>`

## Error Handling

- Do not use exceptions
- Use `Error<T, ErrorType>` to return errors
  - `Error<T, ErrorType>` contains an error string; allocate the error string on the permanent or passed in arena depending on whether the error can be recovered from
  - Success in the `ErrorType` must be represented as `0`
- Use `Optional<T>` for values that may not exist, but not for errors
- Assert conditions with:
  - `DEBUG_ASSERT()` for debug-info compatible builds
  - `ASSERT()` for all builds

## Templates and Generics

- Minimize template complexity
- Don't use templates unless it is necessary
- Use SFINAE sparingly and only when necessary: `std::enable_if_t<std::is_integral_v<T>>`
- Prefer explicit instantiation over heavy template metaprogramming

## Atomics
- For C compatiable APIs: use `atomic_i64`, `atomic_u64`, etc. typedefs or C11's `_Atomic(T)`
- In C++: use the Atomic<T> wrapper
- Use explicit memory ordering: C11 `memory_order` constants directly

## String Handling

- UTF-8 by default with explicit conversion utilities
- Null-termination as optional feature, not requirement
- Slice operations return views, copy operations allocate

## Performance Guidelines

- Mark functions you truly want inlined with `CXB_INLINE`. This will force an inline when no debug information should be generated.
- Mark pure functions with `CXB_PURE`
- Use `LIKELY()`/`UNLIKELY()` for branch prediction hints
- Compile-time evaluation with `CXB_COMPTIME` where possible or use C++'s `constexpr`

## Platform Compatibility

- Use feature detection macros for platform-specific code
- Provide fallbacks for missing features
- Support both C and C++ compilation modes
- Use standard integer types with explicit sizes: `u32`, `i64`
