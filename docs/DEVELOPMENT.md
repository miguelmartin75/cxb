# Development

## Manual Building and Testing

```bash
# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Run tests
cd build && ctest --output-on-failure
# OR run test executable directly
./build/test_seq
```

## Code Formatting

This project uses clang-format for consistent code formatting. See `.clang-format`.

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

The project includes GitHub Actions workflows that:
- Check code formatting on pull requests
- Build and test the project on multiple configurations (e.g. Debug/Release)
- Run on both pushes to main and PRs

All pull requests must pass formatting checks and tests before merging.
