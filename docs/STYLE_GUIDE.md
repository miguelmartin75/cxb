# CXB Style Guide
- Provide C compatible code where it is possible

## Functions
- Mark pure functions with `CXB_PURE`
- Mark functions you truly want inlined with `CXB_INLINE`. This will force an inline when no debug information should be generated.
- Prefer free form functions over member functions
- When performing an operation (verb) on a type instance (noun), free form functions should have the namking convention `<noun>_<verb>` 
- The exception to the above is for:
    - common operations performed across many different types, e.g. `make_<type>`,
    - overloaded functions such as `serialize(TypeA&)` and `serialize(TypeB&)`
- If the function operates on a C-compatible POD type, do not use overloading (obviously)

## Control Flow
- Use `LIKELY()`/`UNLIKELY()` for branch prediction hints

## Assertions, Errors
- Assert conditions with:
  - `DEBUG_ASSERT()` for debug-info compatible builds
  - `ASSERT()` for all builds
- In C++-land 
    - Do not use exceptions
    - Use `Result<T, ErrorType>` to return errors
        - `Result<T, ErrorType>` contains an error string; allocate the error string on the permanent, or an passed in if the error can be recovered
        - Success in the `ErrorType` must be represented by the integer `0`
    - Use `Optional<T>` for values that may not exist, but not for errors
- For C compatible APIs, you can declare a struct:
    - For Result:
        ```cpp
        DECALRE_RESULT_TYPE(ResultFoo, Foo, ErrorCode);

        // equivalent to:
        struct ResultFoo { 
            Foo f; 
            ErrorCode error; 
            StringSlice reason; 

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
    - TODO: should I always do it this way?

## Primitive Types

- Prefer typedef's with explicit bit size over builtin primitives, such as u8, i32, i64, etc.

## Composite Types

- Design around Zero Is Initialization (ZII), if possible
- Prefer POD types (no ctors or dtors), if possible
- Do not use `private` or `protected`, ever. `private` and `protected` is a lie.
- Implement member functions as free form functions when possible. Exceptions include ctors/dtors, assignment/move operators, etc.
    - Use `tools/metadecls.cpp` to automatically generate member functions that call the free form functions to emulate UFCS
    - Include the member functions in the type with `#include src/<type>.members"

## Memory Management & Containers

Memory is managed with either an arena or the general heap.

- Prefer allocating memory with an `Arena`
    - access `perm_arena` to get the permanent arena, call `setup_perm_arena` at the start of your program
    - call `get_scratch()` to get access to a temporary arena
- Container naming convention:
    - `M` prefix indicates that manual memory management is mandatory, use `.destroy()` to free memory
    - `A` prefix indicates that the container will automatically call `.destroy()` using RAII
- Containers utilizing an owned (or existing) arena
    - `MStableArray<T>`, `AStableArray<T>`:
    - `MPool<T>`, `APool<T>`

## Templates and Generics

- Minimize template complexity
- Don't use templates unless it is necessary
- Use SFINAE sparingly and only when necessary: `std::enable_if_t<std::is_integral_v<T>>`
- Prefer explicit instantiation over heavy template metaprogramming

## Parallel Code 
- Atomics
    - For C compatible APIs: use `atomic_i64`, `atomic_u64`, etc. typedefs or C11's `_Atomic(T)`
    - In C++: use the Atomic<T> wrapper
    - Use explicit memory ordering: C11 `memory_order` constants directly

## Names, Keywords

### Macros
- All uppercase with underscores: `CXB_INLINE`, `ASSERT`, `BREAKPOINT`
- Library-specific macros prefixed with `CXB_`: `CXB_EXPORT`, `CXB_INTERNAL`

### Types
- Prefer the `struct` keyword over `class`
- PascalCase for classes and structs: `Allocator`, `Mallocator`, `Utf8Iterator`
- Lowercase abbreviations for primitive type aliases: `f32`, `f64`, `i32`, `u64`
- Descriptive compound names: `Vec2f`, `Color4i`, `Utf8DecodeResult`

### Functions and Variables
- `snake_case` for functions
- `snake_case` for member variables: `n_active_bytes`, `bytes_consumed`

### Constants and Enums
- PascalCase for enum values: `Relaxed`, `Acquire`, `SeqCst`
- snake_case for static constants: `identity4x4`, `identity3x3`
