# cxb

Current Header Compile Times:
* `cxb.h`: 52ms on 2020 MBP (i5 Quad Core)
    * TODO: run in CI and obtain this number

## Development

### Quick Setup

For first-time setup, you can use the automated setup script:

```bash
./setup-dev.sh
```

This script will:
- Install required dependencies (cmake, clang-format, build tools)
- Set up Catch2 v3
- Build the project
- Run tests
- Optionally install the pre-commit hook

### Manual Building and Testing

```bash
# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Run tests
cd build && ctest --output-on-failure
# OR run test executable directly
./build/test_seq
```

### Code Formatting

This project uses clang-format for consistent code formatting. The configuration is defined in `.clang-format`.

#### Format all files
```bash
./scripts/format.sh
```

#### Check formatting without making changes
```bash
./scripts/format.sh --check
```

#### Format specific files manually
```bash
clang-format -i path/to/file.cpp
```

#### Install pre-commit hook (optional)
```bash
cp hooks/pre-commit .git/hooks/pre-commit && chmod +x .git/hooks/pre-commit
```

This will automatically check formatting before each commit and prevent commits with formatting issues.

### CI/CD

The project includes GitHub Actions workflows that:
- Check code formatting on pull requests
- Build and test the project on multiple configurations (Debug/Release)
- Run on both pushes to main/master and pull requests

All pull requests must pass formatting checks and tests before merging.
