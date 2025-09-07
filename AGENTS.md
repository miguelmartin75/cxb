# AGENTS

This repository houses the cxb C++ utility library and its tests.

## Layout
- `cxb/` – core library headers and sources
- `tests/` – unit tests

## Development notes
- Use `rg` for searching the codebase.
- Before building tests, fetch submodules:
  `git submodule update --init --recursive`
- Build and run tests:
  - `cmake -S . -B build -DCXB_BUILD_TESTS=ON`
  - `cmake --build build`
  - `ctest --test-dir build`
- Avoid the C++ standard library when cxb provides equivalent functionality (e.g., use `String8` instead of `std::string`).

