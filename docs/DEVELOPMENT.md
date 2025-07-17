# Development

## Manual Building and Testing

```bash
# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Run tests
cd build && ctest --output-on-failure
# OR run test executable directly
./build/test_arena
```

## Code Formatting

This project uses clang-format for consistent code formatting. See `.clang-format`.

All pull requests must pass formatting checks and tests before merging.

### Format all files
```bash
./scripts/format.sh
```

### Check formatting without making changes
```bash
./scripts/format.sh --check
```

### Format specific files manually
```bash
clang-format -i path/to/file.cpp
```

### Install pre-commit hook (optional)
```bash
cp scripts/hooks/pre-commit .git/hooks/pre-commit && chmod +x .git/hooks/pre-commit
```

This will automatically check formatting before each commit and prevent commits with formatting issues.

## CI/CD

NixOS is used for the CI, which enables debugging the CI easier. See [.github/workflows/ci.yml](../.github/workflows/ci.yml) and [scripts/ci/](../scripts/ci)

Steps performed on CI:
- Format code
- Run tests across x86_64, aarch64 on MacOS and Linux (Ubuntu)
