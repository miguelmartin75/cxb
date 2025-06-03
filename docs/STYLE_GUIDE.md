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
- PascalCase for classes and structs: `Allocator`, `Mallocator`, `Utf8Iterator`
- Lowercase abbreviations for primitive type aliases: `f32`, `f64`, `i32`, `u64`
- Descriptive compound names: `Vec2f`, `Color4i`, `Utf8DecodeResult`

### Functions and Variables
- snake_case for functions: `utf8_decode()`, `growth_sug()`, `to_c11_order()`
- snake_case for member variables: `n_active_bytes`, `bytes_consumed`
- Descriptive names over abbreviations: `codepoint` not `cp`, `allocator` not `alloc`

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

Always use the struct keyword and expose all implementation details. Never use protected or private unless it is strictly necessary for implementation purposes.

```cpp
struct ExampleClass {
    // Member variables first
    Type member_var;

    // Constructors
    ExampleClass() = default;
    ExampleClass(const Type& param) = default;

    void public_method();

    // operators
    ExampleClass& operator=(const ExampleClass& other);
    bool operator==(const ExampleClass& other);
};
```

## Memory Management

- Always use `Allocator*` for custom allocation
- Provide both allocating and non-allocating versions where appropriate
- RAII with explicit `.destroy()` methods for cleanup
- Support for headers in allocations: `alloc_with_header()`, `free_with_header()`

## Error Handling

- Prefer explicit error states over exceptions
- Use `Optional<T>` for values that may not exist
- Assert preconditions with:
  - `ASSERT()` for debug only builds
  - `REQUIRES()` for all builds

## Templates and Generics

- Minimize template complexity
- Use SFINAE sparingly and only when necessary: `std::enable_if_t<std::is_integral_v<T>>`
- Prefer explicit instantiation over heavy template metaprogramming

## Atomic Operations

- Wrap platform atomics in consistent interface
- Provide both C11 `_Atomic` and C++ `std::atomic` backends
- Use explicit memory ordering: `MemoryOrderOption` enum
- Support common atomic operations: `fetch_add`, `compare_exchange_weak`, etc.

## String Handling

- UTF-8 by default with explicit conversion utilities
- Null-termination as optional feature, not requirement
- Slice operations return views, copy operations allocate

## Performance Guidelines

- Mark trivial functions with `CXB_INLINE`
- Mark functions to be inlined but have span multiple lines with `CXB_INTERNAL_NODEBUG`
- Mark pure functions with `CXB_PURE`
- Use `LIKELY()`/`UNLIKELY()` for branch prediction hints
- Compile-time evaluation with `CXB_COMPTIME` where possible

## Platform Compatibility

- Use feature detection macros for platform-specific code
- Provide fallbacks for missing features
- Support both C and C++ compilation modes
- Use standard integer types with explicit sizes: `u32`, `i64`
